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
#ifndef SWRI_CONSOLE_LOG_WIDGET_H_
#define SWRI_CONSOLE_LOG_WIDGET_H_

#include <deque>
#include <vector>

#include <QAbstractScrollArea>
#include <QColor>

#include <swri_console/database_view.h>
#include <swri_console/constants.h>

QT_BEGIN_NAMESPACE;
class QStyleOptionViewItemV4;
QT_END_NAMESPACE;

namespace swri_console
{
class Log;
class LogDatabase;
class LogFilter;
class LogWidget : public QAbstractScrollArea
{
  Q_OBJECT;

 public:
  LogWidget(QWidget *parent=0);
  ~LogWidget();

  void setDatabase(LogDatabase *db);

  LogFilter* logFilter() { return filter_; }

  bool autoScrollToBottom() const { return auto_scroll_to_bottom_; }

  // Returns a view of the selected logs in the widget.
  DatabaseView selectedLogContents() const;
  // Returns a view of all the logs displayed by this widget.
  DatabaseView displayedLogContents() const;
  // Returns a view of all the logs for the sessions displayed by this widget.
  DatabaseView sessionsLogContents() const;
  // Returns a view of all the logs in the database.
  DatabaseView allLogContents() const;

 Q_SIGNALS:
  void autoScrollToBottomChanged(bool auto_scroll);
  void messagesAdded();
  
 public Q_SLOTS:
  void setSessionFilter(const std::vector<int> &sids);
  void setAutoScrollToBottom(bool auto_scroll);
  void setStampFormat(const StampFormat &format);
  void setDebugColor(const QColor &color);
  void setInfoColor(const QColor &color);
  void setWarnColor(const QColor &color);
  void setErrorColor(const QColor &color);
  void setFatalColor(const QColor &color);

  void selectAll();
  void copyLogsToClipboard();
  void copyExtendedLogsToClipboard();

 private:  // members
  LogDatabase *db_;
  LogFilter *filter_;

  bool auto_scroll_to_bottom_;

  StampFormat stamp_format_;
  QColor debug_color_;
  QColor info_color_;
  QColor warn_color_;
  QColor error_color_;
  QColor fatal_color_;

  // We store the displayed logs as individual lines for performance
  // reasons (much easier/faster to treat the contents as a collection
  // of uniform items).  The RowMap struct is used to map our row
  // indices to the log & line that it represents.
  struct RowMap {
    size_t log_index;
    int line_index;
    RowMap() : log_index(0), line_index(0) {}
    RowMap(size_t log, int line) : log_index(log), line_index(line) {}
  };

  struct SessionData
  {
    int session_id;

    size_t latest_log_index;
    std::deque<RowMap> rows;

    size_t earliest_log_index;
    std::deque<RowMap> early_rows;

    // This is what some people might call "too pedantic", but using
    // the built in list view's alternating color caused irritating
    // flashing while old messages were being added to the front of
    // the list.  To get around this, we explicitly track a fixed
    // point and base our own alternating colors off it to get stable
    // coloring.
    int alternate_base;
  };
  std::vector<SessionData> blocks_;
  size_t row_count_;
  int row_height_;

  // A list of session ids that are used to calculate the current
  // message counts.
  std::vector<int> sids_;

 private:  // methods
  bool event(QEvent *);
  void paintEvent(QPaintEvent *);
  void timerEvent(QTimerEvent *);
  void focusInEvent(QFocusEvent *event);
  void focusOutEvent(QFocusEvent *event);
  QVariant toolTipRole(const Log &log, int line_index) const;
  QVariant extendedLogRole(const Log &log, int line_index) const;

  QString logText(const Log &log, int line_index) const;
  const QColor& logColor(const Log &log) const;
    
  void scheduleIdleProcessing();
  void updateRowCount(size_t row_count);                                      
  void updateRowHeight();

  QStyleOptionViewItemV4 viewOptions();
  void fillOption(QStyleOptionViewItemV4 &option, const SessionData &, int row_id);
                                      
 private Q_SLOTS:
  void reset();
  void allDataChanged();
  void processOldMessages();
  void handleDatabaseCleared();
  void processNewMessages();
  void filterModified();

  
};  // class LogWidget
}  // namespace swri_console
#endif  // SWRI_CONSOLE_LOG_WIDGET_H_

