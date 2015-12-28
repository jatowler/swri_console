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
#include <vector>

#include <QTimer>
#include <QDebug>

#include <swri_console/log_list_model.h>
#include <swri_console/log_database.h>

namespace swri_console
{
LogListModel::LogListModel(QObject *parent)
  :
  QAbstractListModel(parent),
  db_(NULL)
{
}

LogListModel::~LogListModel()
{
}

void LogListModel::setDatabase(LogDatabase *db)
{
  if (db_) {
    // We can implement this, just don't have a use case currently.
    qWarning("LogListModel: Cannot change the database.");
    return;
  }

  db_ = db;
  
  // QObject::connect(db_, SIGNAL(logAdded(int)),
  //                  this, SLOT(handleLogAdded(int)));
  // QObject::connect(db_, SIGNAL(logDeleted(int)),
  //                  this, SLOT(handleLogDeleted(int)));
  // QObject::connect(db_, SIGNAL(logRenamed(int)),
  //                  this, SLOT(handleLogRenamed(int)));
  // QObject::connect(db_, SIGNAL(logMoved(int)),
  //                  this, SLOT(handleLogMoved(int)));

  reset();  

  // We're going to update the model on a timer instead of using
  // a new messages signal to limit the update rate.
  startTimer(20);
}

int LogListModel::rowCount(const QModelIndex &parent) const
{
  int size = 0;
  for (auto const &it : blocks_) {
    size += it.lines.size();
  }
  
  return size;
}

QVariant LogListModel::data(const QModelIndex &index, int role) const
{
  if (index.parent().isValid()) {
    return QVariant();
  } 

  int session_idx = -1;
  int line_idx = index.row();

  for (size_t i = 0; i < blocks_.size(); i++) {
    if (line_idx < blocks_[i].lines.size()) {
      session_idx = i;
      break;
    }
    line_idx -= blocks_[i].lines.size();
  }

  if (session_idx < 0) {
    return QVariant();
  }
  
  int sid = blocks_[session_idx].session_id;
  LineMap line_map = blocks_[session_idx].lines[line_idx];
  size_t lid = line_map.log_index;
  Log log = db_->session(sid).log(lid);
  
  if (!log.isValid()) {
    return QVariant();
  }

  if (role == Qt::DisplayRole) {
    return log.textLines()[line_map.line_index];
  } else {
    return QVariant();
  }

  return QVariant();
}

// void LogListModel::handleLogAdded(int sid)
// {
//   std::vector<int> new_logs = db_->logIds();
//   for (size_t i = 0; i < new_logs.size(); i++) {
//     if (new_logs[i] == sid) {
//       beginInsertRows(QModelIndex(), i, i);
//       logs_.swap(new_logs);
//       endInsertRows();
//       return;
//     }
//   }

//   qWarning("Missing SID error in LogListModel::handleLogAdded (%d)", sid);
// }

void LogListModel::setSessionFilter(const std::vector<int> &sids)
{
  sids_ = sids;
  // Note: We could do a partial reset here...
  reset();
}

void LogListModel::reset()
{
  beginResetModel();

  blocks_.clear();
  for (size_t i = 0; i < sids_.size(); i++) {
    const Session &session = db_->session(sids_[i]);
    if (!session.isValid()) {
      qWarning("LogListModel::reset got invalid session %d", sids_[i]);
      continue;
    }

    blocks_.emplace_back();
    SessionData &block = blocks_.back();    
    block.session_id = sids_[i];
    block.latest_log_index = session.logCount();
    block.earliest_log_index = session.logCount();
  }

  endResetModel();

  scheduleIdleProcessing();
}

void LogListModel::scheduleIdleProcessing()
{
  // If we have older logs that still need to be processed, schedule a
  // callback at the next idle time.
  for (size_t i = 0; i < blocks_.size(); i++) {
    if (blocks_[i].earliest_log_index > 0) {
      QTimer::singleShot(0, this, SLOT(processOldMessages()));
      return;
    }
  }
}

void LogListModel::processOldMessages()
{
  // We process old messages in two steps.  First, we process the
  // remaining messages in chunks and store them in a early_lines
  // buffer if they pass all the filters.  When the early mapping
  // buffer is large enough (or we have processed everything for that
  // session), then we merge the early_lines buffer in the main
  // buffer.  This approach allows us to process very large logs
  // without causing major lag for the user.
  //
  // Unlike processNewMessages, we only process old messages for one
  // session at time.  This is because the number of unprocess old
  // messages is bounded, so we will eventually get through all of
  // them.  We could process all of them, but then each processing
  // time chunk would be grow with how many sessions are selected, and
  // we risk making the GUI unresponsive is a lot of sessions are active.
  for (size_t i = 0; i < blocks_.size(); i++) {
    SessionData &block = blocks_[i];
    
    if (block.earliest_log_index == 0) {
      // Nothing to do for this block.
      continue;
    }
    
    const Session &session = db_->session(block.session_id);
    if (!session.isValid()) {
      qWarning("Invalid session in %s?", __PRETTY_FUNCTION__);
      continue;
    }

    for (size_t i = 0;
         block.earliest_log_index != 0 && i < 100;
         block.earliest_log_index--, i++)
    {
      const Log log = session.log(block.earliest_log_index-1);
      // if (!acceptLogEntry(log)) {
      //   continue;
      // }
      
      QStringList text_lines = log.textLines();
      for (int r = 0; r < text_lines.size(); r++) {
        // Note that we have to add the lines backwards to maintain the proper order.
        block.early_lines.push_front(LineMap(block.earliest_log_index-1,
                                             text_lines.size()-1-r));
      }
    }

    if ((block.earliest_log_index == 0 && block.early_lines.size()) ||
        block.early_lines.size() > 200) {
      size_t start_row = 0;
      for (size_t r = 0; r < i; r++) {
        start_row += blocks_[r].lines.size();
      }

      beginInsertRows(QModelIndex(),
                      start_row,
                      start_row + block.early_lines.size() - 1);
      block.lines.insert(block.lines.begin(),
                         block.early_lines.begin(),
                         block.early_lines.end());
      block.early_lines.clear();
      endInsertRows();

      Q_EMIT messagesAdded();
    }

    // Don't process any more blocks.
    break;
  }

  scheduleIdleProcessing();
}

void LogListModel::processNewMessages()
{  
  bool messages_added = false;
  for (size_t i = 0; i < blocks_.size(); i++) {
    SessionData &block = blocks_[i];
    const Session &session = db_->session(block.session_id);
    if (!session.isValid()) {
      qWarning("Invalid session in %s?", __PRETTY_FUNCTION__);
      continue;
    }

    std::deque<LineMap> new_items;
    for(; block.latest_log_index < session.logCount(); block.latest_log_index++) {
      const Log log = session.log(block.latest_log_index);

      // if (!acceptLogEntry(log)) {
      //   continue;
      // }

      QStringList text_lines = log.textLines();
      for (int r = 0; r < text_lines.size(); r++) {
        new_items.push_back(LineMap(block.latest_log_index, r));
      }
    }

    if (!new_items.empty()) {
      size_t start_row = 0;
      for (size_t r = 0; r <= i; r++) {
        start_row += blocks_[r].lines.size();
      }
      
      beginInsertRows(QModelIndex(),
                      start_row,
                      start_row + new_items.size() - 1);
      block.lines.insert(block.lines.end(),
                         new_items.begin(),
                         new_items.end());
      endInsertRows();
      messages_added = true;
    }
  }

  if (messages_added) {
    Q_EMIT messagesAdded();
  }
}

void LogListModel::timerEvent(QTimerEvent*)
{
  processNewMessages();
}
}  // namespace swri_console
