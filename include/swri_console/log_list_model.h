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
#ifndef SWRI_CONSOLE_LOG_LIST_MODEL_H_
#define SWRI_CONSOLE_LOG_LIST_MODEL_H_

#include <string>
#include <deque>
#include <vector>

#include <QAbstractListModel>
#include <QColor>
#include <swri_console/constants.h>

namespace swri_console
{
class LogDatabase;
class LogFilter;
class Log;
class LogListModel : public QAbstractListModel
{
  Q_OBJECT
  
 public:
  LogListModel(QObject *parent=0);
  ~LogListModel();

  void setDatabase(LogDatabase *db);  

  virtual int rowCount(const QModelIndex &parent) const;
  virtual QVariant data(const QModelIndex &index, int role) const;

  LogFilter* logFilter() { return filter_; }

  void setTimeDisplay(const TimeDisplaySetting &value);

  QColor severityColor(const uint8_t severity) const;
  void setSeverityColor(const uint8_t severity, const QColor &color);

  void setSessionFilter(const std::vector<int> &sids);
    
 Q_SIGNALS:
  void messagesAdded();

 private Q_SLOTS:
  void reset();
  void allDataChanged();
  void processOldMessages();
  
 private:
  void scheduleIdleProcessing();
  void timerEvent(QTimerEvent *);
  void processNewMessages();

  QVariant displayRole(const Log &log, int line_index) const;
  QVariant toolTipRole(const Log &log, int line_index) const;
  QVariant foregroundRole(const Log &log, int line_index) const;
  QVariant extendedLogRole(const Log &log, int line_index) const;
  
  LogDatabase *db_;
  LogFilter *filter_;

  TimeDisplaySetting time_display_;
  QColor debug_color_;
  QColor info_color_;
  QColor warn_color_;
  QColor error_color_;
  QColor fatal_color_;
  
  // For performance reasons, the proxy model presents single line
  // items, while the underlying log database stores multi-line
  // messages.  The LineMap struct is used to map our item indices to
  // the log & line that it represents.
  struct LineMap {
    size_t log_index;
    int line_index;
    LineMap() : log_index(0), line_index(0) {}
    LineMap(size_t log, int line) : log_index(log), line_index(line) {}
  };

  struct SessionData
  {
    int session_id;

    size_t latest_log_index;
    std::deque<LineMap> lines;

    size_t earliest_log_index;
    std::deque<LineMap> early_lines;
  };
  std::vector<SessionData> blocks_;

  // A list of session ids that are used to calculate the current
  // message counts.
  std::vector<int> sids_;
};
}  // namespace swri_console
#endif  // SWRI_CONSOLE_LOG_LIST_MODEL_H_
