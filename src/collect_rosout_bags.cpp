// *****************************************************************************
//
// Copyright (c) 2015, Southwest Research Institute® (SwRI®)
// All rights reserved.
//
// Redistribution and use in source and binary forms, with or without
// modification, are permitted provided that the following conditions are met:
//     * Redistributions of source code must retain the above copyright
//       notice, this list of conditions and the following disclaimer.
//     * Redistributions in binary form must reproduce the above copyright
//       notice, this list of conditions and the following disclaimer in the
//       documentation and/or other materials provided with the distribution.
//     * Neither the name of Southwest Research Institute® (SwRI®) nor the
//       names of its contributors may be used to endorse or promote products
//       derived from this software without specific prior written permission.
//
// THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS"
// AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
// IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
// ARE DISCLAIMED. IN NO EVENT SHALL Southwest Research Institute® BE LIABLE 
// FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL 
// DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR 
// SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER 
// CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT 
// LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY 
// OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH
// DAMAGE.
//
// *****************************************************************************
#include <signal.h>

#include <QCoreApplication>
#include <QStringList>
#include <QFile>
#include <QTextStream>
#include <QDebug>

#include <rosbag/bag.h>
#include <rosbag/view.h>
#include <rosgraph_msgs/Log.h>

/**
 * This program collects rosout log messages from an arbitrary number
 * of bag files and put them into a single bag file.  I use to it make
 * super huge bag files that are useful for stress testing.
 */

bool quit = false;

void hupSignalHandler(int)
{
  quit = true;
}

void termSignalHandler(int)
{
  quit = true;
}

int setupSignalHandlers()
{
  struct sigaction hup, term;

  hup.sa_handler = hupSignalHandler;
  sigemptyset(&hup.sa_mask);
  hup.sa_flags = 0;
  hup.sa_flags |= SA_RESTART;

  if (sigaction(SIGHUP, &hup, 0) > 0)
    return 1;

  term.sa_handler = termSignalHandler;
  sigemptyset(&term.sa_mask);
  term.sa_flags |= SA_RESTART;

  if (sigaction(SIGTERM, &term, 0) > 0)
    return 2;

  return 0;
}

QStringList sourceFilesFromText(const QString &text_filename)
{
  QFile file(text_filename);

  if (!file.exists()) {
    qWarning("File %s does not exist.", qPrintable(text_filename));
    return QStringList();
  }

  if (!file.open(QIODevice::ReadOnly | QIODevice::Text)) {
    qWarning("Failed to open file %s", qPrintable(text_filename));
    return QStringList();
  }

  QStringList files;
  QTextStream in(&file);
  while (!in.atEnd()) {
    QString line = in.readLine().trimmed();

    if (line.isEmpty() || line[0] == QChar('#')) {
      continue;
    }
    
    files.append(line);
  }

  return files;
}

bool processArgs(QString &dst_file, QStringList &src_files)
{
  QStringList args = QCoreApplication::arguments();

  if (args.size() < 3) {
    qWarning("Not enough arguments: destination.bag source1.(bag|txt) ...");
    return false;
  }

  dst_file = args[1];
  
  bool error = false;  
  for (int i = 2; i < args.size(); i++) {
    if (args[i].endsWith(".bag")) {
      src_files.append(args[i]);
    } else if (args[i].endsWith(".txt")) {
      QStringList file_list = sourceFilesFromText(args[i]);
      if (!file_list.isEmpty()) {
        src_files = src_files + file_list;
      } else {
        qWarning("Error reading file %s", qPrintable(args[i]));
        error = true;
      }
    } else {
      qWarning("Unknown argument %s", qPrintable(args[i]));
      error = true;
    }
  }
      
  if (error) {
    src_files.clear();
    return false;
  }

  return true;
}


bool chooseTopic(QString &topic, rosbag::Bag &bag)
{
  QStringList topics;
  topics.append("/rosout_agg");
  topics.append("rosout_agg");
  topics.append("/rosout");
  topics.append("rosout");

  for (int i = 0; i < topics.size(); i++) {
    auto conns = rosbag::View(bag, rosbag::TopicQuery(topics[i].toStdString())).getConnections();
    if (conns.empty()) {
      continue;
    }
    
    // Can't figure out how to get message count from c++ api?
    topic = topics[i];
    return true;
  }

  return false;
}

int main(int argc, char **argv)
{
  QCoreApplication app(argc, argv);
  setupSignalHandlers();
  
  QString dst_fn;
  QStringList src_fns;
  if (!processArgs(dst_fn, src_fns)) {
    return -1;
  }

  rosbag::Bag dst_bag(dst_fn.toStdString(), rosbag::bagmode::Write);
  dst_bag.setCompression(rosbag::compression::BZ2);

  bool first_ever = true;
  ros::Time last_time(ros::TIME_MAX);
  ros::Duration time_adjustment(0);

  for (int i = 0; i < src_fns.size(); i++) {
    if (quit) { break; }
    size_t msg_count = 0;
    try {
      rosbag::Bag src_bag(src_fns[i].toStdString(), rosbag::bagmode::Read);

      QString topic;

      if (!chooseTopic(topic, src_bag)) {
        qWarning("%04d/%d: Skipping %s", i+1, src_fns.size(), qPrintable(src_fns[i]));
        continue;
      }
    
      qWarning("%04d/%d: Importing messages from %s in %s",
               i+1, src_fns.size(), qPrintable(topic), qPrintable(src_fns[i]));

      bool first_in_bag = true;
    
      rosbag::View view(src_bag, rosbag::TopicQuery(topic.toStdString()));
      for (auto it = view.begin(); it != view.end(); it++) {
        if (quit) { break; }
        auto src_log = it->instantiate<rosgraph_msgs::Log>();

        if (first_ever) {
          time_adjustment = ros::Duration(0);
        } else if (first_in_bag) {
          // Put a 10 second gap between files.
          time_adjustment = last_time  - src_log->header.stamp + ros::Duration(10.0);
        } else {
          ros::Time new_stamp = src_log->header.stamp + time_adjustment;
          if (new_stamp - last_time > ros::Duration(5.0)) {
            // Bound long stretches for radio silence to 1 second.        
            time_adjustment = last_time + ros::Duration(1.0) - src_log->header.stamp;
          } else if (new_stamp - last_time < ros::Duration(-0.5)) {
            time_adjustment = last_time + ros::Duration(0.001) - src_log->header.stamp;
          }
        } 
        
        last_time = src_log->header.stamp + time_adjustment;
        first_ever = false;
        first_in_bag = false;

        rosgraph_msgs::Log dst_log;
        dst_log.header.seq = src_log->header.seq;
        dst_log.header.stamp = last_time;
        dst_log.header.frame_id = src_log->header.frame_id;
        dst_log.level = src_log->level;
        dst_log.name = src_log->name;
        dst_log.msg = src_log->msg;
        dst_log.file = src_log->file;
        dst_log.function = src_log->function;
        dst_log.line = src_log->line;
        // Ignore topics... don't know what they were thinking when they included it.
        // dst_log.topics = src_log.topics;
        dst_bag.write("/rosout_agg", last_time, dst_log);
        msg_count++;      
      }

      qWarning("   found %zu messages.", msg_count);
    } catch (rosbag::BagException &e) {
      qWarning("Exception occurred accessing %s after %zu messages.  Continuing to next file.",
               qPrintable(src_fns[i]), msg_count);
    }     
  }
  
  return 0;
}
