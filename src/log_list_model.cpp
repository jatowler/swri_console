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
#include <swri_console/log_list_model.h>

#include <vector>

#include <QTimer>
#include <QDebug>

#include <swri_console/log_database.h>
#include <swri_console/log_filter.h>

namespace swri_console
{
LogListModel::LogListModel(QObject *parent)
  :
  QAbstractListModel(parent),
  db_(NULL),
  filter_(new LogFilter(this)),
  time_display_(RELATIVE_TIME),
  debug_color_(Qt::gray),
  info_color_(Qt::black),
  warn_color_(QColor(255,127,0)),
  error_color_(Qt::red),
  fatal_color_(Qt::magenta)
{
  QObject::connect(filter_, SIGNAL(filterModified()),
                   this, SLOT(reset()));                   
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

  QObject::connect(db_, SIGNAL(sessionMinTimeChanged(int)),
                   this, SLOT(allDataChanged()));
  
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
    size += it.rows.size();
  }
  
  return size;
}

bool LogListModel::decomposeModelIndex(int &session_idx, int &row_idx,
                                       const QModelIndex index) const
{
  session_idx = -1;
  row_idx = index.row();

  for (size_t i = 0; i < blocks_.size(); i++) {
    if (row_idx < blocks_[i].rows.size()) {
      session_idx = i;
      break;
    }
    row_idx -= blocks_[i].rows.size();
  }

  if (session_idx < 0) {
    return false;
  }

  return true;
}

QVariant LogListModel::data(const QModelIndex &index, int role) const
{
  switch (role)
  {
    // Currently we're only returning data for these roles, so return immediately
    // if we're being queried for anything else.
    case Qt::DisplayRole:
    case Qt::ToolTipRole:
    case Qt::ForegroundRole:
    case Qt::BackgroundRole:
    case ExtendedLogRole:
      break;
    default:
      return QVariant();
  }

  if (index.parent().isValid()) {
    return QVariant();
  } 

  int session_idx;
  int row_idx;
  if (!decomposeModelIndex(session_idx, row_idx, index)) {
    return QVariant();
  }
    
  int sid = blocks_[session_idx].session_id;
  RowMap line_map = blocks_[session_idx].rows[row_idx];
  size_t lid = line_map.log_index;
  Log log = db_->session(sid).log(lid);
  
  if (!log.isValid()) {
    return QVariant();
  }

  if (role == Qt::DisplayRole) {
    return displayRole(log, line_map.line_index);
  } else if (role == Qt::ToolTipRole) {
    return toolTipRole(log, line_map.line_index);
  } else if (role == Qt::ForegroundRole) {
    return foregroundRole(log, line_map.line_index);
  } else if (role == Qt::BackgroundRole) {
    return backgroundRole(session_idx, row_idx);
  } else if (role == ExtendedLogRole) {
    return extendedLogRole(log, line_map.line_index);
  } else {
    return QVariant();
  }

  return QVariant();
}

QVariant LogListModel::displayRole(const Log &log, int line_index) const
{  
  char severity = '?';
  if (log.severity() == rosgraph_msgs::Log::DEBUG) {
    severity = 'D';
  } else if (log.severity() == rosgraph_msgs::Log::INFO) {
    severity = 'I';
  } else if (log.severity() == rosgraph_msgs::Log::WARN) {
    severity = 'W';
  } else if (log.severity() == rosgraph_msgs::Log::ERROR) {
    severity = 'E';
  } else if (log.severity() == rosgraph_msgs::Log::FATAL) {
    severity = 'F';
  }

  QString header;
  if (time_display_ == NO_TIME) {
    header = QString("[%1] ").arg(severity);
  } else if (time_display_ == RELATIVE_TIME) {
    ros::Time t = log.relativeTime();
    int32_t secs = t.sec;
    int hours = secs / 60 / 60;
    int minutes = (secs / 60) % 60;
    int seconds = (secs % 60);
    int milliseconds = t.nsec / 1000000;

    char stamp[128];
    snprintf(stamp, sizeof(stamp),
             "%d:%02d:%02d:%03d",
             hours, minutes, seconds, milliseconds);
    
    header = QString("[%1 %2] ").arg(severity).arg(stamp);
  } else {
    ros::Time t = log.absoluteTime();
    char stamp[128];
    snprintf(stamp, sizeof(stamp), 
             "%u.%09u",
             t.sec,
             t.nsec);
    header = QString("[%1 %2] ").arg(severity).arg(stamp);
  }

  // For multiline messages, we only want to display the header for
  // the first line.  For the subsequent lines, we generate a header
  // and then fill it with blank lines so that the messages are
  // aligned properly (assuming monospaced font).  
  if (line_index != 0) {
    header.fill(' ');
  }
    
  return QString(header) + log.textLines()[line_index];
}

QVariant LogListModel::toolTipRole(const Log &log, int line_index) const
{           
  QString text("<p style='white-space:pre'>");
  text += extendedLogRole(log, line_index).toString();
  text += QString("</p>");
  return text;
}

QVariant LogListModel::extendedLogRole(const Log &log, int line_index) const
{
  char stamp[128];
  snprintf(stamp, sizeof(stamp),
           "%d.%09d",
           log.absoluteTime().sec,
           log.absoluteTime().nsec);

  QString text;
  text += QString("Timestamp: %1\n").arg(stamp);
  text += QString("Node: %1\n").arg(log.nodeName());
  text += QString("Function: %1\n").arg(log.functionName());
  text += QString("File: %1\n").arg(log.fileName());
  text += QString("Line: %1\n").arg(log.lineNumber());
  text += QString("\n");
  text += log.textLines().join("\n");
  return text;
}

QVariant LogListModel::foregroundRole(const Log &log, int) const
{
  return severityColor(log.severity());
}

QVariant LogListModel::backgroundRole(int session_idx, int row_idx) const
{
  if ((row_idx - blocks_[session_idx].alternate_base) % 2) {
    return QColor(240, 240, 240);
  } else {
    return QVariant();
  }
}

void LogListModel::setSessionFilter(const std::vector<int> &sids)
{
  sids_ = sids;
  // Note: We could do a partial reset here...
  reset();
}

void LogListModel::setTimeDisplay(const TimeDisplaySetting &value)
{
  if (time_display_ == value) { return; }
  time_display_ = value;
  allDataChanged();
}

QColor LogListModel::severityColor(const uint8_t severity) const
{
  switch (severity) {
  case rosgraph_msgs::Log::DEBUG:
    return debug_color_;
  case rosgraph_msgs::Log::INFO:
    return info_color_;
  case rosgraph_msgs::Log::WARN:
    return warn_color_;
  case rosgraph_msgs::Log::ERROR:
    return error_color_;
  case rosgraph_msgs::Log::FATAL:
    return fatal_color_;
  default:
    return info_color_;
  }
}  

void LogListModel::setSeverityColor(const uint8_t severity, const QColor &color)
{
  if (severity == rosgraph_msgs::Log::DEBUG) {
    debug_color_ = color;
  } else if (severity == rosgraph_msgs::Log::INFO) {
    info_color_ = color;
  } else if (severity == rosgraph_msgs::Log::WARN) {
    warn_color_ = color;
  } else if (severity == rosgraph_msgs::Log::ERROR) {
    error_color_ = color;
  } else if (severity == rosgraph_msgs::Log::FATAL) {
    fatal_color_ = color;
  } else {
    qWarning("Attempt to set color for invalid severity: %d", severity);
  }
  allDataChanged();
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
    block.alternate_base = 0;
  }

  endResetModel();

  scheduleIdleProcessing();
}

void LogListModel::allDataChanged()
{
  Q_EMIT dataChanged(index(0), index(rowCount(QModelIndex())-1));
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
  // remaining messages in chunks and store them in a early_rows
  // buffer if they pass all the filters.  When the early mapping
  // buffer is large enough (or we have processed everything for that
  // session), then we merge the early_rows buffer in the main
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
      if (!filter_->accept(log)) {
         continue;
      }
      
      QStringList text_lines = log.textLines();
      for (int r = 0; r < text_lines.size(); r++) {
        // Note that we have to add the lines backwards to maintain the proper order.
        block.early_rows.push_front(RowMap(block.earliest_log_index-1,
                                            text_lines.size()-1-r));
      }
    }

    if ((block.earliest_log_index == 0 && block.early_rows.size()) ||
        block.early_rows.size() > 200) {
      size_t start_row = 0;
      for (size_t r = 0; r < i; r++) {
        start_row += blocks_[r].rows.size();
      }

      beginInsertRows(QModelIndex(),
                      start_row,
                      start_row + block.early_rows.size() - 1);
      block.rows.insert(block.rows.begin(),
                        block.early_rows.begin(),
                        block.early_rows.end());
      block.alternate_base += block.early_rows.size();
      block.early_rows.clear();
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

    std::deque<RowMap> new_items;
    for(; block.latest_log_index < session.logCount(); block.latest_log_index++) {
      const Log log = session.log(block.latest_log_index);

      if (!filter_->accept(log)) {
        continue;
      }

      QStringList text_lines = log.textLines();
      for (int r = 0; r < text_lines.size(); r++) {
        new_items.push_back(RowMap(block.latest_log_index, r));
      }
    }

    if (!new_items.empty()) {
      size_t start_row = 0;
      for (size_t r = 0; r <= i; r++) {
        start_row += blocks_[r].rows.size();
      }
      
      beginInsertRows(QModelIndex(),
                      start_row,
                      start_row + new_items.size() - 1);
      block.rows.insert(block.rows.end(),
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

void LogListModel::reduceIndices(QModelIndexList &indices)
{
  // This is irritatingly complex.  This takes a sorted list of model
  // indices and reduces it so that there is one index per log (since
  // multiple line logs have multiple indices).  Currently this is
  // needed by the copy extended logs function.
    
  int last_session_idx = -1;
  size_t last_log_idx = 0;
  int dropped = 0;
  
  for (int i = 0; i < indices.size(); i++) {
    int session_idx;
    int row_idx;
   
    if (!decomposeModelIndex(session_idx, row_idx, indices[i])) {
      // Should not happen
      dropped++;
    }

    // One MEELLION indirects...
    size_t log_idx = blocks_[session_idx].rows[row_idx].log_index;
    int line_idx = blocks_[session_idx].rows[row_idx].line_index;

    if (session_idx == last_session_idx && log_idx == last_log_idx) {
      dropped++;
    } else {
      last_session_idx = session_idx;
      last_log_idx = log_idx;

      if (dropped != 0) {
        indices[i-dropped] = indices[i];
      }
    }
  }

  while (dropped > 0) {
    indices.removeLast();
    dropped--;
  }
}

void LogListModel::timerEvent(QTimerEvent*)
{
  processNewMessages();
}
}  // namespace swri_console
