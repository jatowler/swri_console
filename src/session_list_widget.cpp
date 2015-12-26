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
#include <swri_console/session_list_widget.h>

#include <algorithm>
#include <set>

#include <QListWidget>
#include <QVBoxLayout>
#include <QDebug>

#include <swri_console/log_database.h>

namespace swri_console
{
SessionListWidget::SessionListWidget(QWidget *parent)
  :
  QWidget(parent),
  db_(NULL)
{
  list_widget_ = new QListWidget(this);
  list_widget_->setFont(QFont("Ubuntu Mono", 9));
  list_widget_->setContextMenuPolicy(Qt::CustomContextMenu);

  auto *main_layout = new QVBoxLayout();  
  main_layout->addWidget(list_widget_);
  main_layout->setContentsMargins(0,0,0,0);
  setLayout(main_layout);
}

SessionListWidget::~SessionListWidget()
{
}

void SessionListWidget::setDatabase(LogDatabase *db)
{
  if (db_) {
    // We can implement this if needed, just don't have a current use case.
    qWarning("SessionListWidget: Cannot change the log database.");
    return;
  }

  db_ = db;
  synchronize();
  startTimer(50);
}

void SessionListWidget::timerEvent(QTimerEvent*)
{
  synchronize();
}

void SessionListWidget::synchronize()
{
  std::set<int> prev_set;
  prev_set.insert(sessions_.begin(), sessions_.end());

  const std::vector<int>& sessions = db_->sessionIds();
  std::set<int> next_set;
  next_set.insert(sessions.begin(), sessions.end());
    
  std::set<int> removed_sessions;
  std::set_difference(prev_set.begin(), prev_set.end(),
                      next_set.begin(), next_set.end(),
                      std::inserter(removed_sessions, removed_sessions.end()));
    
  std::set<int> added_sessions;
  std::set_difference(next_set.begin(), next_set.end(),
                      prev_set.begin(), prev_set.end(),
                      std::inserter(added_sessions, added_sessions.end()));

  size_t removed = 0;
  for (size_t i = 0; i < sessions_.size(); i++) {
    if (removed_sessions.count(sessions_[i])) {
      QListWidgetItem *item = list_widget_->takeItem(i - removed);
      delete item;
      removed++;
    }
  }

  sessions_ = sessions;
  for (size_t i = 0; i < sessions_.size(); i++) {
    int id = sessions_[i];
    if (added_sessions.count(id)) {
      QListWidgetItem *item = new QListWidgetItem();
      item->setText("");
      item->setFlags(item->flags() | Qt::ItemIsEditable);
      list_widget_->insertItem(i, item);
    }

    const Session &session = db_->session(id);
    QListWidgetItem *item = list_widget_->item(i);
    item->setData(Qt::DisplayRole, QString("%1 (%2)")
                  .arg(session.name())
                  .arg(session.messageCount()));
    //item->setData(Qt::EditRole, session.name());
  }
}
}  // namespace swri_console
