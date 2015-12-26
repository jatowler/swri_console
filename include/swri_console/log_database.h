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
#ifndef SWRI_CONSOLE_LOG_DATABASE_H_
#define SWRI_CONSOLE_LOG_DATABASE_H_

#include <deque>
#include <unordered_map>
#include <vector>

#include <QObject>
#include <QStringList>

#include <ros/time.h>
#include <rosgraph_msgs/Log.h>

#include <swri_console/session.h>

namespace swri_console
{
struct LogEntry
{
  ros::Time stamp;
  uint8_t level;  
  std::string node;  
  std::string file;
  std::string function;
  uint32_t line;
  QStringList text;
  uint32_t seq;
};

class LogDatabase : public QObject
{
  Q_OBJECT;

 public:
  LogDatabase();
  ~LogDatabase();

  void clear();
  const std::deque<LogEntry>& log() { return log_; }
  const ros::Time& minTime() const { return min_time_; }

  const std::map<std::string, size_t>& messageCounts() const { return msg_counts_; }

  int createSession(const QString &name);
  Session& session(int id) { sessions_.count(id) ? sessions_.at(id) : invalid_session_; }
  const Session& session(int id) const  { sessions_.count(id) ? sessions_.at(id) : invalid_session_; }
  const std::vector<int>& sessionIds() const { return session_ids_; }
  
  void addMessage(int run_id, const rosgraph_msgs::LogConstPtr &msg);

 Q_SIGNALS:
  void databaseCleared();
  void minTimeUpdated();

 public Q_SLOTS:
  void queueMessage(const rosgraph_msgs::LogConstPtr msg);

 private:
  std::map<std::string, size_t> msg_counts_;
  std::deque<LogEntry> log_;

  ros::Time min_time_;

  std::unordered_map<int, Session> sessions_;
  std::vector<int> session_ids_;
  const static Session invalid_session_;

  friend class Session;
};  // class LogDatabase
}  // namespace swri_console 
#endif  // SWRI_CONSOLE_LOG_DATABASE_H_
