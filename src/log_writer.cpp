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

#include <QFile>
#include <QTextStream>

#include <swri_console/save_file_dialog.h>
#include <swri_console/log_widget.h>
#include <swri_console/log_database.h>
#include <swri_console/session.h>
#include <swri_console/log.h>

#include <rosbag/bag.h>
#include <rosgraph_msgs/Log.h>

namespace swri_console
{
LogWriter::LogWriter(QObject *parent)
  :
  QObject(parent),
  db_(NULL)  
{
}

LogWriter::~LogWriter()
{
}

void LogWriter::setDatabase(LogDatabase *db)
{
  if (db_) {
    // We can implement this if needed, just don't have a current use case.
    qWarning("LogWriter: Cannot change the log database.");
    return;
  }

  db_ = db;
}

void LogWriter::save(LogWidget *log_list)
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

  if (dialog.selectedFiles().empty()) {
    return;
  }
  const QString filename = dialog.selectedFiles()[0];
  
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

  if (filename.endsWith(".bag", Qt::CaseInsensitive)) {
    saveBagFile(filename, view, dialog.sessionHeaders(), dialog.compression());
  } else {
    saveTextFile(filename, view, dialog.sessionHeaders(), dialog.extendedInfo());
  }
}
  
void LogWriter::saveBagFile(const QString &filename,
                            const DatabaseView &view,
                            bool session_header,
                            bool compression) const
{
  rosbag::Bag bag(filename.toStdString().c_str(), rosbag::bagmode::Write);
  if (compression) {
      bag.setCompression(rosbag::compression::BZ2);
  }

  for (const SessionView &session_view : view) {
    const Session &session = db_->session(session_view.session_id);
    if (!session.isValid()) {
      qWarning("saveBagFile: Skipping invalid session %d", session_view.session_id);
    }

    if (session_header) {
      rosgraph_msgs::Log msg;
      msg.header.seq = 0;
      msg.header.stamp = session.minTime();
      msg.header.frame_id = "__swri_console_session_separator__";
      msg.level = rosgraph_msgs::Log::INFO;
      msg.name = session.name().toStdString();
      msg.msg = std::string("The following messages were collected from ") + session.name().toStdString();
      msg.file = session.name().toStdString();
      msg.function = "__swri_console_session_separator__";
      msg.line = 0;
      bag.write("/rosout_agg", msg.header.stamp, msg);
    }

    for (auto const &lid : session_view.log_ids) {
      const Log& log = session.log(lid);

      rosgraph_msgs::Log msg;
      msg.header.seq = lid;
      msg.header.stamp = log.absoluteTime();
      msg.header.frame_id = "__swri_console__";
      msg.level = log.severity();
      msg.name = log.nodeName().toStdString();
      msg.msg = log.textLines().join("\n").toStdString();
      msg.file = log.fileName().toStdString();
      msg.function = log.functionName().toStdString();
      msg.line = log.lineNumber();
      bag.write("/rosout_agg", msg.header.stamp, msg);
    }
  }
  bag.close();
}

void LogWriter::saveTextFile(const QString &filename,
                             const DatabaseView &view,
                             bool session_header,
                             bool extended_info) const
{
  QFile out_file(filename);
  out_file.open(QFile::WriteOnly);
  QTextStream outstream(&out_file);

  for (const SessionView &session_view : view) {
    const Session &session = db_->session(session_view.session_id);
    if (!session.isValid()) {
      qWarning("saveBagFile: Skipping invalid session %d", session_view.session_id);
    }

    if (session_header) {
      outstream << "----------------------------------------\n";
      outstream << "-- " << session.name() << "\n";
      outstream << "----------------------------------------\n";      
    }

    for (auto const &lid : session_view.log_ids) {
      const Log& log = session.log(lid);

      if (extended_info) {
        outstream << QString("[%1]: ").arg(log.nodeName());
      }
      outstream << log.textLines().join("\n") << "\n";
    }
  }

  outstream.flush();
  out_file.close();
}
}  // namespace swri_console

