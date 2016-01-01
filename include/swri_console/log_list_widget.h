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
#ifndef SWRI_CONSOLE_LOG_LIST_WIDGET_H_
#define SWRI_CONSOLE_LOG_LIST_WIDGET_H_

#include <QWidget>
#include <QModelIndexList>

#include <unordered_map>

#include <swri_console/constants.h>
#include <swri_console/database_view.h>

QT_BEGIN_NAMESPACE
class QListView;
QT_END_NAMESPACE

namespace swri_console
{
class LogDatabase;
class LogFilter;
class LogListModel;
class LogListWidget : public QWidget
{
  Q_OBJECT;

 public:
  LogListWidget(QWidget *parent=0);
  ~LogListWidget();

  void setDatabase(LogDatabase *db);

  LogFilter* logFilter();
  
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
                                    
 private Q_SLOTS:
  void handleMessagesAdded();
  void userScrolled(int);
    
 private:
  QModelIndexList selection() const;
  DatabaseView viewForSids(const std::vector<int> &sids) const;
  
  LogDatabase *db_;
  LogListModel *model_;
  QListView *list_view_;

  bool auto_scroll_to_bottom_;  
};  // class LogListWidget
}  // namespace swri_console
#endif  // SWRI_CONSOLE_LOG_LIST_WIDGET_H_
