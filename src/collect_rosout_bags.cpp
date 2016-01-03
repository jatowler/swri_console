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
#include <QDir>
#include <QFile>
#include <QFileInfo>
#include <QTextStream>
#include <QDebug>
#include <QTime>

#include <rosbag/bag.h>
#include <rosbag/view.h>
#include <rosgraph_msgs/Log.h>

/**
 * This program extracts rosout log messages from an arbitrary number
 * of bag files of bag files, putting them into bag files in a common
 * directory.  Useful for stress testing.
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

bool processArgs(QString &dst_dir, QStringList &src_files)
{
  QStringList args = QCoreApplication::arguments();

  if (args.size() < 3) {
    qWarning("Not enough arguments: destination.bag source1.(bag|txt) ...");
    return false;
  }

  dst_dir = args[1];
  
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


bool chooseTopic(QString &topic, size_t &count, rosbag::Bag &bag)
{
  QStringList topics;
  topics.append("/rosout_agg");
  topics.append("rosout_agg");
  topics.append("/rosout");
  topics.append("rosout");

  for (int i = 0; i < topics.size(); i++) {
    rosbag::View view(bag, rosbag::TopicQuery(topics[i].toStdString()));
    if (view.getConnections().empty()) {
      continue;
    }
    
    topic = topics[i];
    count = view.size();
    return true;
  }

  return false;
}

int main(int argc, char **argv)
{
  QCoreApplication app(argc, argv);
  setupSignalHandlers();
  
  QString dst_dir;
  QStringList src_fns;
  if (!processArgs(dst_dir, src_fns)) {
    return -1;
  }

  if (!QDir().mkpath(dst_dir)) {
    qWarning("Failed to make directory %s", qPrintable(dst_dir));
    return -1;
  }

  qWarning("Writing files to %s", qPrintable(dst_dir));

  QTime timer;
  timer.start();
  
  size_t total_msg_count = 0;
  for (int i = 0; i < src_fns.size(); i++) {
    if (quit) { break; }

    char header[1024];
    snprintf(header, sizeof(header), "[% 11zu %04d/%04d] ",
             total_msg_count, i+1, src_fns.size());
        

    size_t msg_count = 0;
    try {
      rosbag::Bag src_bag(src_fns[i].toStdString(), rosbag::bagmode::Read);
      
      QString topic;
      size_t expected_msg_count = 0;

      if (!chooseTopic(topic, expected_msg_count, src_bag)) {
        qWarning("%s Skipping %s", header, qPrintable(src_fns[i]));
        continue;
      }
      
      QFileInfo src_file_info(src_fns[i]);
      QFileInfo dst_file_info(dst_dir, "rosout-" + src_file_info.fileName());
      QString dst_fn = dst_file_info.absoluteFilePath();
    
      rosbag::Bag dst_bag(dst_fn.toStdString(), rosbag::bagmode::Write);
      dst_bag.setCompression(rosbag::compression::BZ2);
    
      qWarning("%s Importing %zu, messages from %s in %s",
               header, expected_msg_count, qPrintable(topic), qPrintable(src_fns[i]));
      
      rosbag::View view(src_bag, rosbag::TopicQuery(topic.toStdString()));
      for (auto it = view.begin(); it != view.end(); it++) {
        if (quit) { break; }
        auto src_log = it->instantiate<rosgraph_msgs::Log>();
          
        rosgraph_msgs::Log dst_log;
        dst_log.header.seq = src_log->header.seq;
        dst_log.header.stamp = src_log->header.stamp;
        dst_log.header.frame_id = src_log->header.frame_id;
        dst_log.level = src_log->level;
        dst_log.name = src_log->name;
        dst_log.msg = src_log->msg;
        dst_log.file = src_log->file;
        dst_log.function = src_log->function;
        dst_log.line = src_log->line;
        // Ignore topics... not useful and take up lots of space
        // dst_log.topics = src_log.topics;
        dst_bag.write("/rosout_agg", it->getTime(), dst_log);
        msg_count++;      
        total_msg_count++;
      }
    } catch (rosbag::BagException &e) {
      qWarning("Exception occurred accessing %s after %zu messages.  Continuing to next file.",
               qPrintable(src_fns[i]), msg_count);
    }     
  }
  
  return 0;
}
