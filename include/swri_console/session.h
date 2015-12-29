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
#ifndef SWRI_CONSOLE_SESSION_H_
#define SWRI_CONSOLE_SESSION_H_

#include <deque>
#include <unordered_map>

#include <QString>

#include <ros/time.h>
#include <rosgraph_msgs/Log.h>

#include <swri_console/log.h>

namespace swri_console
{
class LogDatabase;
class Session
{
 public:
  int id_;
  QString name_;
  LogDatabase *db_;

  ros::Time min_time_;

  std::unordered_map<int, size_t> node_log_counts_;

  friend class Log;
  friend class LogDatabase;

  struct LogData
  {
    ros::Time stamp;
    uint8_t severity;
    int node_id;
    QString file;
    QString function;
    uint32_t line;
    QStringList text_lines;
  };
  std::deque<LogData> log_data_;
    
 public:
  Session();
  ~Session();

  bool isValid() const { return id_ >= 0; }
  const QString& name() const { return name_; }
  const size_t nodeLogCount(int nid) const { return node_log_counts_.count(nid) ? node_log_counts_.at(nid) : 0; }

  void append(const rosgraph_msgs::LogConstPtr &msg);

  const int logCount() const { return static_cast<int>(log_data_.size()); }
  Log log(int index) const {
    if (index < 0 || index >= logCount()) {
      return Log();
    } else {
      return Log(this, index);
    }    
  }
};  // class Session
}  // namespace swri_console
#endif  // SWRI_CONSOLE_SESSION_H_
