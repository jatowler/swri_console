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

#include <stdio.h>

#include <ros/time.h>
#include <rosbag/bag.h>

#include <swri_console/log_database_proxy_model.h>
#include <swri_console/log_database.h>

#include <QColor>
#include <QFile>
#include <QTextStream>
#include <QTimer>
#include <QSettings>
#include <swri_console/settings_keys.h>

namespace swri_console
{
LogDatabaseProxyModel::LogDatabaseProxyModel(LogDatabase *db)
{
  QObject::connect(db_, SIGNAL(databaseCleared()),
                   this, SLOT(handleDatabaseCleared()));
}

QSettings settings;
settings.setValue(SettingsKeys::ABSOLUTE_TIMESTAMPS, display_absolute_time_);
settings.setValue(SettingsKeys::DISPLAY_TIMESTAMPS, display_time_);
QSettings settings;
settings.setValue(SettingsKeys::DEBUG_COLOR, debug_color);
settings.setValue(SettingsKeys::INFO_COLOR, info_color);
settings.setValue(SettingsKeys::WARN_COLOR, warn_color);
settings.setValue(SettingsKeys::ERROR_COLOR, error_color);
settings.setValue(SettingsKeys::FATAL_COLOR, fatal_color);

void LogDatabaseProxyModel::saveToFile(const QString& filename) const
{
  if (filename.endsWith(".bag", Qt::CaseInsensitive)) {
    saveBagFile(filename);
  }
  else {
    saveTextFile(filename);
  }
}

void LogDatabaseProxyModel::saveBagFile(const QString& filename) const
{
  rosbag::Bag bag(filename.toStdString().c_str(), rosbag::bagmode::Write);

  size_t idx = 0;
  while (idx < msg_mapping_.size()) {
    const LineMap line_map = msg_mapping_[idx];    
    const LogEntry &item = db_->log()[line_map.log_index];
    
    rosgraph_msgs::Log log;
    log.file = item.file;
    log.function = item.function;
    log.header.seq = item.seq;
    if (item.stamp < ros::TIME_MIN) {
      // Note: I think TIME_MIN is the minimum representation of
      // ros::Time, so this branch should be impossible.  Nonetheless,
      // it doesn't hurt.
      log.header.stamp = ros::Time::now();
      qWarning("Msg with seq %d had time (%d); it's less than ros::TIME_MIN, which is invalid. "
               "Writing 'now' instead.",
               log.header.seq, item.stamp.sec);
    } else {
      log.header.stamp = item.stamp;
    }
    log.level = item.level;
    log.line = item.line;
    log.msg = item.text.join("\n").toStdString();
    log.name = item.node;
    bag.write("/rosout", log.header.stamp, log);

    // Advance to the next line with a different log index.
    idx++;
    while (idx < msg_mapping_.size() && msg_mapping_[idx].log_index == line_map.log_index) {
      idx++;
    }
  }
  bag.close();
}

void LogDatabaseProxyModel::saveTextFile(const QString& filename) const
{
  QFile outFile(filename);
  outFile.open(QFile::WriteOnly);
  QTextStream outstream(&outFile);
  for(size_t i = 0; i < msg_mapping_.size(); i++)
  {
    QString line = data(index(i), Qt::DisplayRole).toString();
    outstream << line << '\n';
  }
  outstream.flush();
  outFile.close();
}
}  // namespace swri_console
