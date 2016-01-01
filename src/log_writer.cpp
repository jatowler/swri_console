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
#include <swri_console/log_writer.h>

#include <swri_console/save_file_dialog.h>
#include <swri_console/log_list_widget.h>

namespace swri_console
{
LogWriter::LogWriter(QObject *parent)
  :
  QObject(parent)
{
}

LogWriter::~LogWriter()
{
}

void LogWriter::saveLogs(LogListWidget *log_list)
{
  // QString defaultname = QDateTime::currentDateTime().toString(Qt::ISODate) + ".bag";
  // QString filename = QFileDialog::getSaveFileName(this,
  //                                                 "Save Logs",
  //                                                 QDir::homePath() + QDir::separator() + defaultname,
  //                                                 tr("Bag Files (*.bag);;Text Files (*.txt)"));
  // if (filename != NULL && !filename.isEmpty()) {
  //   // db_proxy_->saveToFile(filename);
  // }
  SaveFileDialog dialog;
  if (dialog.exec() != QDialog::Accepted) {
    return;
  }

  QStringList files = dialog.selectedFiles();
  if (files.empty()) {
    return;
  }

  QString filename = files[0];
  
  DatabaseView view;
  if (dialog.exportAll()) {
    view = log_list->allLogContents();
  } else if (dialog.exportSessions()) {
    view = log_list->sessionsLogContents();
  } else if (dialog.exportFiltered()) {
    view = log_list->displayedLogContents();
  } else if (dialog.exportSelected()) {
    view = log_list->selectedLogContents();
  } else {
    qWarning("No export selection, will not write bag file.");
  }  
}
}  // namespace swri_console

