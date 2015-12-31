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

#include <QMimeData>
#include <QDebug>

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
  QObject::connect(db_, SIGNAL(sessionMoved(int)),
                   this, SLOT(handleSessionMoved(int)));
  QObject::connect(db_, SIGNAL(databaseCleared()),
                   this, SLOT(handleDatabaseCleared()));

  beginResetModel();
  sessions_ = db_->sessionIds();
  endResetModel();
  
  startTimer(20);
}

int SessionListModel::sessionId(const QModelIndex &index) const
{
  if (index.parent().isValid() || index.row() > sessions_.size()) {
    return -1;
  }

  return sessions_[index.row()];
}

int SessionListModel::rowCount(const QModelIndex &parent) const
{
  return sessions_.size();
}

Qt::ItemFlags SessionListModel::flags(const QModelIndex &index) const
{
  return (Qt::ItemIsSelectable |
          Qt::ItemIsEditable |
          Qt::ItemIsEnabled |
          Qt::ItemIsDragEnabled |
          Qt::ItemIsDropEnabled);
}

Qt::DropActions SessionListModel::supportedDropActions() const
{
  return Qt::MoveAction;
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

bool SessionListModel::setData(const QModelIndex &index, const QVariant &value, int role)
{
  if (index.parent().isValid() || index.row() > sessions_.size()) {
    return false;
  } 

  int sid = sessions_[index.row()];
  db_->renameSession(sid, value.toString());
  return true;
}

void SessionListModel::handleSessionAdded(int sid)
{
  std::vector<int> new_sessions = db_->sessionIds();
  for (size_t i = 0; i < new_sessions.size(); i++) {
    if (new_sessions[i] == sid) {
      beginInsertRows(QModelIndex(), i, i);
      sessions_.swap(new_sessions);
      endInsertRows();

      Q_EMIT sessionAdded(index(i));
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

void SessionListModel::handleSessionMoved(int sid)
{
  std::vector<int> new_sessions = db_->sessionIds();
  if (new_sessions.size() != sessions_.size()) {
    qWarning("Unexpected session size mismatch (%zu vs %zu)",
             new_sessions.size(),
             sessions_.size());
    return;
  }
  
  int src_index = -1;
  int dst_index = -1;
  for (size_t i = 0; i < sessions_.size(); i++) {
    if (sid == sessions_[i]) {
      src_index = i;
    }
    if (sid == new_sessions[i]) {
      dst_index = i;
    }
  }

  if (src_index < 0) {
    qWarning("Could not find session id in existing index.");
    return;
  }
  if (dst_index < 0) {
    qWarning("Could not find session id in updated index.");
    return;
  }

  if (dst_index == src_index) {
    return;
  }

  if (dst_index < src_index) {
    beginMoveRows(QModelIndex(), src_index, src_index, QModelIndex(), dst_index);
  } else {
    beginMoveRows(QModelIndex(), src_index, src_index, QModelIndex(), dst_index+1);
  }    
  sessions_.swap(new_sessions);
  endMoveRows();
}

void SessionListModel::timerEvent(QTimerEvent*)
{
  Q_EMIT dataChanged(index(0), index(sessions_.size()-1));
}

bool SessionListModel::dropMimeData(const QMimeData *data,
                                    Qt::DropAction action,
                                    int dst_row, int dst_column,
                                    const QModelIndex &parent)
{  
  // We ALWAYS return false even if we succeeded so that the view
  // doesn't try to delete the source row itself.  It won't actually
  // matter since we don't implement the removeRows() behavior at this
  // point, but that could change.  It's irritating that the
  // implementation of a move is split between a view and a model, but
  // that's how it is.

  if (!data || !(action == Qt::CopyAction || action == Qt::MoveAction))
    return false;
  QStringList types = mimeTypes();
  if (types.isEmpty())
    return false;
  QString format = types.at(0);
  if (!data->hasFormat(format))
    return false;

  // We have to unpack the data from the MIME object to get a list of
  // the source rows.
  QVector<int> src_rows;
  QByteArray encoded = data->data(format);
  QDataStream stream(&encoded, QIODevice::ReadOnly);
  while (!stream.atEnd()) {
    int r, c; // we discard the column
    QMap<int,QVariant> data; // and we discard the data
    stream >> r >> c >> data;
    src_rows.append(r);
  }

  // Sort the rows so that after they are inserted, they will
  // have the same relative ordering.
  qSort(src_rows);

  // Convert the rows in to session ids.
  QVector<int> src_ids;
  for (int i = 0; i < src_rows.size(); i++) {
    src_ids.append(sessions_[src_rows[i]]);
  }

  // I never expected supporting drag & drop to be so complicated.
  
  int target_row = -1;
  if (!parent.isValid()) {
    // If the parent is not valid, the user dropped the selection in
    // between rows.
    if (dst_row < 0) {
      // qDebug() << "selection dropped at end of list.";
      target_row = sessions_.size()-1;
    } else {
      // qDebug() << "selection dropped after item " << sessions_[dst_row-1];
      target_row = dst_row-1;
    }
  } else {
    // qDebug() << "selection dropped on item " << sessions_[parent.row()];
    if (src_rows[0] < parent.row()) {
      target_row = parent.row();
    } else {
      target_row = parent.row()-1;
    }
  }

  // qDebug() << "target_row = " << target_row;
  int target_id = -1;
  for (int i = target_row; i >= 0; i--) {
    if (!src_ids.contains(sessions_[i])) {
      target_id = sessions_[i];
      break;
    }
  }
  // If no item was found, that means we'll place the selection at
  // the front of the list.
  // qDebug() << "target_id = " << target_id;
  
  // The most straightforward case to handle this in the general case
  // (arbitrary, discontinuous selections) is to move the first
  // element so that it is in the target position, move the second
  // element so that it is after the first element. move the third
  // element so that it is after the second element, and so on.  We're
  // going to take the most straightforward approach, even though it's
  // inefficient, because I don't want to deal with all the special
  // logic of "if this element is below the target then X, but if it's
  // below then Y, etc.
  QVector<int> dst_ids;
  dst_ids.append(target_id);
  for (size_t i = 1; i < src_ids.size(); i++) {
    dst_ids.append(src_ids[i-1]);
  }

  for (size_t i = 0; i < src_ids.size(); i++) {
    db_->moveSession(src_ids[i], dst_ids[i]);
  }
    
  return false;
}

void SessionListModel::handleDatabaseCleared()
{
  beginResetModel();
  sessions_.clear();
  endResetModel();
}
}  // namespace swri_console
