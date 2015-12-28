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

#include <stdio.h>
#include <vector>

#include <swri_console/session_list_model.h>
#include <swri_console/log_database.h>

namespace swri_console
{
SessionListModel::SessionListModel(QObject *parent)
  :
  QAbstractListModel(parent),
  db_(NULL)
{
}

SessionListModel::~SessionListModel()
{
}

void SessionListModel::setDatabase(LogDatabase *db)
{
  if (db_) {
    // We can implement this, just don't have a use case currently.
    qWarning("SessionListModel: Cannot change the database.");
    return;
  }

  db_ = db;
  
  QObject::connect(db_, SIGNAL(sessionAdded(int)),
                   this, SLOT(handleSessionAdded(int)));
  QObject::connect(db_, SIGNAL(sessionDeleted(int)),
                   this, SLOT(handleSessionDeleted(int)));
  QObject::connect(db_, SIGNAL(sessionRenamed(int)),
                   this, SLOT(handleSessionRenamed(int)));

  beginResetModel();
  sessions_ = db_->sessionIds();
  endResetModel();
  
  startTimer(20);
}

int SessionListModel::rowCount(const QModelIndex &parent) const
{
  return sessions_.size();
}

int SessionListModel::sessionId(const QModelIndex &index) const
{
  if (index.parent().isValid() || index.row() > sessions_.size()) {
    return -1;
  }

  return sessions_[index.row()];
}

QVariant SessionListModel::data(const QModelIndex &index, int role) const
{
  if (index.parent().isValid() || index.row() > sessions_.size()) {
    return QVariant();
  } 

  const Session &session = db_->session(sessions_[index.row()]);
  if (!session.isValid()) {
    return QVariant();
  }

  if (role == Qt::DisplayRole) {
    return QString("%1 (%2)")
      .arg(session.name())
      .arg(session.logCount());
  } else if (role == Qt::EditRole) {
    return session.name();
  } else {
    return QVariant();
  }

  return QVariant();
}

void SessionListModel::handleSessionAdded(int sid)
{
  std::vector<int> new_sessions = db_->sessionIds();
  for (size_t i = 0; i < new_sessions.size(); i++) {
    if (new_sessions[i] == sid) {
      beginInsertRows(QModelIndex(), i, i);
      sessions_.swap(new_sessions);
      endInsertRows();
      return;
    }
  }

  qWarning("Missing SID error in SessionListModel::handleSessionAdded (%d)", sid);
}

void SessionListModel::handleSessionDeleted(int sid)
{
  std::vector<int> new_sessions = db_->sessionIds();
  for (size_t i = 0; i < sessions_.size(); i++) {
    if (sessions_[i] == sid) {
      beginRemoveRows(QModelIndex(), i, i);
      sessions_.swap(new_sessions);
      endRemoveRows();
      return;
    }
  }

  qWarning("Missing SID error in SessionListModel::handleSessionDeleted (%d)", sid);
}

void SessionListModel::handleSessionRenamed(int sid)
{
  for (size_t i = 0; i < sessions_.size(); i++) {
    if (sessions_[i] == sid) {
      Q_EMIT dataChanged(index(i), index(i));
      return;
    }
  }
}

void SessionListModel::timerEvent(QTimerEvent*)
{
  Q_EMIT dataChanged(index(0), index(sessions_.size()-1));
}
}  // namespace swri_console
