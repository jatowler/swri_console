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

#include <swri_console/node_list_model.h>

#include <QLocale>
#include <vector>

#include <swri_console/log_database.h>

namespace swri_console
{
NodeListModel::NodeListModel(QObject *parent)
  :
  QAbstractListModel(parent),
  db_(NULL)
{
}

NodeListModel::~NodeListModel()
{
}

void NodeListModel::setDatabase(LogDatabase *db)
{
  if (db_) {
    // We can implement this, just don't have a use case currently.
    qWarning("NodeListModel: Cannot change the database.");
    return;
  }

  db_ = db;
  
  QObject::connect(db_, SIGNAL(nodeAdded(int)),
                   this, SLOT(handleNodeAdded(int)));
  QObject::connect(db_, SIGNAL(databaseCleared()),
                   this, SLOT(handleDatabaseCleared()));

  beginResetModel();
  nodes_ = db_->nodeIds();
  updateCountCache();
  endResetModel();
  
  startTimer(20);
}

int NodeListModel::nodeId(const QModelIndex &index) const
{
  if (index.parent().isValid() || index.row() > nodes_.size()) {
    return -1;
  }

  return nodes_[index.row()];
}

int NodeListModel::rowCount(const QModelIndex &parent) const
{
  return nodes_.size();
}

QVariant NodeListModel::data(const QModelIndex &index, int role) const
{
  if (index.parent().isValid() || index.row() > nodes_.size()) {
    return QVariant();
  } 

  int nid = nodes_[index.row()];
  int count = msg_count_cache_[index.row()];

  if (role == Qt::DisplayRole) {
    return QString("%1 (%2)")
      .arg(db_->nodeName(nid))
      .arg(QLocale(QLocale::English).toString(count));
  } else if (role == Qt::ForegroundRole) {
    if (count == 0) {
      // Un-emphasize nodes with no messages.
      return Qt::gray;
    } else {
      // Use default color
      return QVariant();
    }
  }
    
    return QVariant();
}

void NodeListModel::handleNodeAdded(int nid)
{
  std::vector<int> new_nodes = db_->nodeIds();
  for (size_t i = 0; i < new_nodes.size(); i++) {
    if (new_nodes[i] == nid) {
      beginInsertRows(QModelIndex(), i, i);
      nodes_.swap(new_nodes);
      updateCountCache();
      endInsertRows();
      return;
    }
  }

  qWarning("Missing NID error in NodeListModel::handleNodeAdded (%d)", nid);
}


void NodeListModel::timerEvent(QTimerEvent*)
{
  updateCountCache();
  Q_EMIT dataChanged(index(0), index(nodes_.size()-1));
}

void NodeListModel::setSessionFilter(const std::vector<int> &sids)
{
  filter_sids_ = sids;
  updateCountCache();
  Q_EMIT dataChanged(index(0), index(nodes_.size()-1));
}

void NodeListModel::updateCountCache()
{
  msg_count_cache_.resize(nodes_.size());
  for (size_t i = 0; i < nodes_.size(); i++) {
    const int nid = nodes_[i];
    int count = 0;
    for (const int sid : filter_sids_) {
      count += db_->session(sid).nodeLogCount(nid);
    }
    msg_count_cache_[i] = count;
  }    
}

void NodeListModel::handleDatabaseCleared()
{
  beginResetModel();
  nodes_ = db_->nodeIds();
  filter_sids_.clear();
  updateCountCache();
  endResetModel();
}
}  // namespace swri_console

