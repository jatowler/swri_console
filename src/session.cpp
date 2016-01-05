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
#include <swri_console/session.h>
#include <swri_console/log_database.h>

namespace swri_console
{
Session::Session()
  :
  number_re_("[-+]?\\d*\\.?\\d+([eE][-+]?\\d+)?"),
  id_(-1),
  name_("__uninitialized__"),
  db_(NULL),
  min_time_(ros::TIME_MAX)
{
}

Session::~Session()
{
}

void Session::append(const rosgraph_msgs::LogConstPtr &msg)
{
  int nid = db_->lookupNode(msg->name);
  node_log_counts_[nid]++;

  if (msg->header.stamp < min_time_) {
    min_time_ = msg->header.stamp;

    // It seems totally not cool to emit someone else's signal, but
    // it's so simple and doesn't require turning sessions into
    // QObjects.
    Q_EMIT db_->sessionMinTimeChanged(id_);
  }

  QStringList text_lines = QString::fromStdString(msg->msg).split(QRegExp("\n|\r\n|\r"));
  // Remove empty lines from the back.
  while(text_lines.size() && text_lines.back().trimmed().isEmpty()) { text_lines.pop_back(); }
  // Remove empty lines from the front.
  while(text_lines.size() && text_lines.front().trimmed().isEmpty()) { text_lines.pop_front(); }
  QString text = text_lines.join("\n");

  QString proto_text;
  QStringList variables;
  
  int start_idx = 0;
  while (start_idx < text.size()) {
    int match_idx = number_re_.indexIn(text, start_idx);
    if (match_idx == -1) {
      // Thing else in this string.  Copy the remainder and quit.
      proto_text += text.midRef(start_idx, text.size() - start_idx);
      break;
    }

    if (start_idx != match_idx) {
      proto_text += text.midRef(start_idx, match_idx - start_idx);
    }

    proto_text += "0";
    variables.append(
      text.mid(match_idx, number_re_.matchedLength()));
    start_idx = match_idx + number_re_.matchedLength();
  }    

  LogPrototype prototype;
  prototype.severity = msg->level;
  prototype.node_id = nid;
  prototype.file = msg->file;
  prototype.function = msg->function;
  prototype.line = msg->line;
  prototype.text = proto_text.toStdString();

  LogData data;
  data.stamp = msg->header.stamp;
  data.proto_id = db_->lookupPrototype(nid, prototype);
  data.variables = variables.join(" ").toStdString();
  log_data_.push_back(data);
}
}  // namespace swri_console
