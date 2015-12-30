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
#include <swri_console/log_list_widget.h>

#include <algorithm>
#include <set>

#include <QApplication>
#include <QClipboard>
#include <QDebug>
#include <QListView>
#include <QMenu>
#include <QScrollBar>
#include <QVBoxLayout>

#include <swri_console/log_database.h>
#include <swri_console/log_list_model.h>

namespace swri_console
{
LogListWidget::LogListWidget(QWidget *parent)
  :
  QWidget(parent),
  db_(NULL),
  auto_scroll_to_bottom_(true)
{
  model_ = new LogListModel(this);
  
  setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Expanding);

  list_view_ = new QListView(this);
  list_view_->setModel(model_);
  list_view_->setFont(QFont("Ubuntu Mono", 9));
  list_view_->setUniformItemSizes(true);
  // list_view_->setAlternatingRowColors(true);
  
  list_view_->setSelectionBehavior(QAbstractItemView::SelectItems);
  list_view_->setSelectionMode(QAbstractItemView::ExtendedSelection);

  // Turn off the list view's context menu so that Qt will pass
  // context menu requests up to us.
  list_view_->setContextMenuPolicy(Qt::NoContextMenu);  
  

  auto *main_layout = new QVBoxLayout();  
  main_layout->addWidget(list_view_);
  main_layout->setContentsMargins(0,0,0,0);
  setLayout(main_layout);

  // QObject::connect(
  //   list_view_->selectionModel(),
  //   SIGNAL(selectionChanged(const QItemSelection &, const QItemSelection &)),
  //   this,
  //   SLOT(handleViewSelectionChanged()));
  QObject::connect(
    model_, SIGNAL(messagesAdded()),
    this, SLOT(handleMessagesAdded()));

  QObject::connect(
    list_view_->verticalScrollBar(), SIGNAL(valueChanged(int)),
    this, SLOT(userScrolled(int)));  
}

LogListWidget::~LogListWidget()
{
}

void LogListWidget::setDatabase(LogDatabase *db)
{
  if (db_) {
    // We can implement this if needed, just don't have a current use case.
    qWarning("LogListWidget: Cannot change the log database.");
    return;
  }

  db_ = db;
  model_->setDatabase(db_);
}

LogFilter* LogListWidget::logFilter()
{
  return model_->logFilter();
}

void LogListWidget::setSessionFilter(const std::vector<int> &sids)
{
  model_->setSessionFilter(sids);
}

void LogListWidget::setAutoScrollToBottom(bool auto_scroll)
{
  if (auto_scroll_to_bottom_ == auto_scroll) {
    return;
  }

  auto_scroll_to_bottom_ = auto_scroll;
  Q_EMIT autoScrollToBottomChanged(auto_scroll_to_bottom_);

  if (auto_scroll_to_bottom_) {
    list_view_->scrollToBottom();
  }
}

void LogListWidget::setTimeDisplay(const TimeDisplaySetting &value)
{
  model_->setTimeDisplay(value);
}

void LogListWidget::setSeverityColor(const uint8_t severity, const QColor &color)
{
  model_->setSeverityColor(severity, color);
}

void LogListWidget::handleMessagesAdded()
{
  if (auto_scroll_to_bottom_) {
    list_view_->scrollToBottom();
  }
}

void LogListWidget::userScrolled(int value)
{
  if (value == list_view_->verticalScrollBar()->maximum()) {
    setAutoScrollToBottom(true);
  } else {
    setAutoScrollToBottom(false);
  }  
}

void LogListWidget::selectAll()
{
  list_view_->selectAll();
}

void LogListWidget::copyLogsToClipboard()
{
  QModelIndexList selected = list_view_->selectionModel()->selection().indexes();

  // QSelectionModel does not return a sorted set, and it will
  // typically be in the order that the items were selected by the
  // user.  We sort them by their row so that the copy items will have
  // the same physical relationship as the source.
  std::sort(selected.begin(), selected.end(),
            [](const QModelIndex &a, const QModelIndex &b) {
              return a.row() < b.row();
            });
  
  QStringList buffer;
  buffer.reserve(selected.size());

  for (auto const &index : selected) {
    buffer.append(model_->data(index, Qt::DisplayRole).toString());
  }
  QApplication::clipboard()->setText(buffer.join(tr("\n")));
}

void LogListWidget::copyExtendedLogsToClipboard()
{
  QModelIndexList selected = list_view_->selectionModel()->selection().indexes();

  // QSelectionModel does not return a sorted set, and it will
  // typically be in the order that the items were selected by the
  // user.  We sort them by their row so that the copy items will have
  // the same physical relationship as the source.
  std::sort(selected.begin(), selected.end(),
            [](const QModelIndex &a, const QModelIndex &b) {
              return a.row() < b.row();
            });

  model_->reduceIndices(selected);
  
  QStringList buffer;
  buffer.reserve(selected.size());

  for (auto const &index : selected) {
    buffer.append(model_->data(index, LogListModel::ExtendedLogRole).toString());
  }
  QString separator = tr("----------------------------------------\n");
  QApplication::clipboard()->setText(buffer.join(separator));
}
}  // namespace swri_console
