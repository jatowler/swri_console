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
#ifndef SWRI_CONSOLE_SESSION_LIST_DELEGATE_H_
#define SWRI_CONSOLE_SESSION_LIST_DELEGATE_H_

#include <QStyledItemDelegate>

namespace swri_console
{
/* Defines a custom delegate for the session list model and it's view.
 * The entire purpose of this class is to get around an annoying
 * default behavior.  When an item is being edited and is part of a
 * data changed signal, the editor resets its contents to the item's
 * edit role.  Since our items are constantly being updated with new
 * log counts, the editor is effectively locked.  This delegate caches
 * the last value that was assigned to the editor and only updates the
 * editor if the item's EditRole data is different from the previously
 * assigned value. 
 */
class SessionListDelegate : public QStyledItemDelegate
{
  Q_OBJECT;
  
 public:
  SessionListDelegate(QObject *parent=0);
  QWidget* createEditor(QWidget *parent, const QStyleOptionViewItem &option, const QModelIndex &index) const;
  void setEditorData(QWidget *editor, const QModelIndex &index) const;

 private:
  mutable QVariant editor_data_;
};  // class SessionListDelegate
}  // namespace swri_console
#endif  // SWRI_CONSOLE_SESSION_LIST_DELEGATE_H_

