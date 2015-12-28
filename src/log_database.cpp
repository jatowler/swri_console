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
#include <swri_console/log_database.h>
#include <swri_console/session.h>

#include <QDebug>

namespace swri_console
{
LogDatabase::LogDatabase()
  :
  min_time_(ros::TIME_MAX)
{
}

LogDatabase::~LogDatabase()
{
}

void LogDatabase::clear()
{
  std::map<std::string, size_t>::iterator iter;
  msg_counts_.clear();
  log_.clear();
  Q_EMIT databaseCleared();
}

int LogDatabase::createSession(const QString &name)
{
  // Find an available id
  int sid = sessions_.size();
  while (sessions_.count(sid) != 0) { sid++; }

  Session &session = sessions_[sid];
  session.id_ = sid;
  session.name_ = name;
  session.db_ = this;

  session_ids_.push_back(sid);
  Q_EMIT sessionAdded(sid);
  return sid;
}

void LogDatabase::deleteSession(int sid)
{
  if (sessions_.count(sid) == 0) {
    qWarning("Attempt to delete invalid session %d", sid);
    return;
  }

  sessions_.erase(sid);
  auto iter = std::find(session_ids_.begin(), session_ids_.end(), sid);
  if (iter != session_ids_.end()) {
    session_ids_.erase(iter);
  } else {
    qWarning("Unexpected inconsistency: session %d was not found in session id vector.", sid);
  } 

  Q_EMIT sessionDeleted(sid);  
}

void LogDatabase::renameSession(int sid, const QString &name)
{
  // It feels weird that this functionality is provided by LogDatabase
  // instead of the session, but otherwise we have to either emit
  // signals from the session (and deal with how those signals should
  // be connected up), or we have to emit the LogDatabase's signal
  // from Session, which seems icky.  Or we have to provide a method
  // for session to notify the database that the sesion was renamed,
  // at which point we might as well just do the renaming here...
  Session &session = this->session(sid);
  if (!session.isValid()) {
    return;
  }

  session.name_ = name;
  Q_EMIT sessionRenamed(sid);  
}

void LogDatabase::moveSession(int sid, int index)
{
  if (index < 0 || index >= session_ids_.size()) {
    qWarning("Refusing the move session to invalid index (%d)", index);
    return;
  }

  // Find the SID in our vector.
  int src_index = -1;
  for (size_t i = 0; i < session_ids_.size(); i++) {
    if (session_ids_[i] == sid) {
      src_index = i;
      break;
    }
  }

  if (src_index < 0) {
    qWarning("Did not find session %d, cannot move.", sid);
    return;
  }

  if (src_index == index) {
    return;
  }
  
  // Scoot everything after the source forward one.
  for (size_t i = src_index; i+1 < session_ids_.size(); i++) {
    session_ids_[i] = session_ids_[i+1];
  }

  // Scoot everything after the destination back one.
  for (size_t i = index; i+1 < session_ids_.size(); i++) {
    session_ids_[i+1] = session_ids_[i];
  }

  // Put the SID in the right place.
  session_ids_[index] = sid;
  Q_EMIT sessionMoved(sid);
}

Session& LogDatabase::session(int sid)
{
  if (sessions_.count(sid)) {
    return sessions_.at(sid);
  }

  qWarning("Request for invalid session %d", sid);
  invalid_session_ = Session();
  return invalid_session_;
}

const Session& LogDatabase::session(int sid) const
{
  if (sessions_.count(sid)) {
    return sessions_.at(sid);
  }

  qWarning("Request for invalid session %d", sid);
  invalid_session_ = Session();
  return invalid_session_;
}

int LogDatabase::lookupNode(const std::string &name)
{
  if (node_id_from_name_.count(name) == 0) {    
    int nid = node_name_from_id_.size();
    while (node_name_from_id_.count(nid) != 0) { nid++; }

    node_name_from_id_[nid] = QString::fromStdString(name);
    node_id_from_name_[name] = nid;

    // Rebuild the node id vector from the map.  We're using the fact
    // that std::map orders it's keys to get the node names in
    // alphabetical order. 
    node_ids_.clear();
    for (auto const &it : node_id_from_name_) {
      node_ids_.push_back(it.second);
    }
    
    Q_EMIT nodeAdded(nid);
  }

  return node_id_from_name_.at(name);
}

QString LogDatabase::nodeName(int nid) const
{
  if (node_name_from_id_.count(nid)) {
    return node_name_from_id_.at(nid);
  }

  qWarning("Request for invalid node %d", nid);
  return QString("<invalid node %1").arg(nid);
}


// void LogDatabase::queueMessage(const rosgraph_msgs::LogConstPtr msg)
// {
//   if (msg->header.stamp < min_time_) {
//     min_time_ = msg->header.stamp;
//     Q_EMIT minTimeUpdated();
//   }
  
//   msg_counts_[msg->name]++;

//   LogEntry log;
//   log.stamp = msg->header.stamp;
//   log.level = msg->level;
//   log.node = msg->name;
//   log.file = msg->file;
//   log.function = msg->function;
//   log.line = msg->line;
//   log.seq = msg->header.seq;

//   QStringList text = QString(msg->msg.c_str()).split('\n');
//   // Remove empty lines from the back.
//   while(text.size() && text.back().isEmpty()) { text.pop_back(); }
//   // Remove empty lines from the front.
//   while(text.size() && text.front().isEmpty()) { text.pop_front(); }  
//   log.text = text;

//   log_.push_back(log);  
// }
}  // namespace swri_console
