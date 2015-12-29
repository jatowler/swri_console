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
#include <swri_console/node_list_widget.h>

#include <algorithm>
#include <set>

#include <QListView>
#include <QVBoxLayout>
#include <QDebug>

#include <swri_console/log_database.h>
#include <swri_console/node_list_model.h>

namespace swri_console
{
NodeListWidget::NodeListWidget(QWidget *parent)
  :
  QWidget(parent),
  db_(NULL)
{
  model_ = new NodeListModel(this);

  setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Expanding);
  
  list_view_ = new QListView(this);
  list_view_->setModel(model_);
  list_view_->setFont(QFont("Ubuntu Mono", 9));
  list_view_->setUniformItemSizes(true);

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
}

NodeListWidget::~NodeListWidget()
{
}

void NodeListWidget::setDatabase(LogDatabase *db)
{
  if (db_) {
    // We can implement this if needed, just don't have a current use case.
    qWarning("NodeListWidget: Cannot change the log database.");
    return;
  }

  db_ = db;
  model_->setDatabase(db_);
}

void NodeListWidget::setSessionFilter(const std::vector<int> &sids)
{
  model_->setSessionFilter(sids);
}

void NodeListWidget::handleViewSelectionChanged()
{
  QModelIndexList selected = list_view_->selectionModel()->selection().indexes();

  // I'm sorting these just to be consistent with how things are done
  // in the session widget, but it really shouldn't matter here
  // because this selection is just used to filter the logs and has no
  // bearing on the log ordering.
  std::sort(selected.begin(), selected.end(),
            [](const QModelIndex &a, const QModelIndex &b) {
              return a.row() < b.row();
            });

  selected_nids_.resize(selected.size());  
  for (int i = 0; i < selected.size(); i++) {
    selected_nids_[i] = model_->nodeId(selected[i]);
  }

  Q_EMIT selectionChanged(selected_nids_);
}

}  // namespace swri_console
