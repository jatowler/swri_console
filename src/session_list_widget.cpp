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

#include <QListView>
#include <QVBoxLayout>
#include <QDebug>

#include <swri_console/log_database.h>
#include <swri_console/session_list_model.h>
#include <swri_console/session_list_delegate.h>

namespace swri_console
{
SessionListWidget::SessionListWidget(QWidget *parent)
  :
  QWidget(parent),
  db_(NULL)
{
  model_ = new SessionListModel(this);

  setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Expanding);
  
  list_view_ = new QListView(this);
  list_view_->setItemDelegate(new SessionListDelegate(this));
  list_view_->setModel(model_);
  list_view_->setUniformItemSizes(true);
  
  list_view_->setContextMenuPolicy(Qt::CustomContextMenu);
  
  list_view_->setDragDropMode(QAbstractItemView::InternalMove);
  list_view_->setDropIndicatorShown(true);

  list_view_->setSelectionBehavior(QAbstractItemView::SelectItems);
  list_view_->setSelectionMode(QAbstractItemView::ExtendedSelection);  

  auto *main_layout = new QVBoxLayout();  
  main_layout->addWidget(list_view_);
  main_layout->setContentsMargins(0,0,0,0);
  setLayout(main_layout);

  QObject::connect(
    list_view_->selectionModel(),
    SIGNAL(selectionChanged(const QItemSelection &, const QItemSelection &)),
    this,
    SLOT(handleViewSelectionChanged()));

  QObject::connect(model_, SIGNAL(sessionAdded(const QModelIndex &)),
                   this, SLOT(handleSessionAdded(const QModelIndex &)));
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
  model_->setDatabase(db_);
}

void SessionListWidget::handleViewSelectionChanged()
{
  QModelIndexList selected = list_view_->selectionModel()->selection().indexes();

  // We're sorting the selection to be in the same order as the layout
  // in the list because that was the ordering is always visually
  // consistent.  Users can reorder the session with drag & drop to
  // change the layout.
  std::sort(selected.begin(), selected.end(),
            [](const QModelIndex &a, const QModelIndex &b) {
              return a.row() < b.row();
            });

  selected_sids_.resize(selected.size());  
  for (int i = 0; i < selected.size(); i++) {
    selected_sids_[i] = model_->sessionId(selected[i]);
  }

  Q_EMIT selectionChanged(selected_sids_);
}

void SessionListWidget::handleSessionAdded(const QModelIndex &idx)
{
  list_view_->selectionModel()->select(idx, QItemSelectionModel::Select);    
}
}  // namespace swri_console
