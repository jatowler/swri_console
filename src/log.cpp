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
#include <swri_console/log.h>
#include <swri_console/session.h>
#include <swri_console/log_database.h>

namespace swri_console
{
ros::Time Log::absoluteTime() const
{
  if (!session_) { return ros::Time(0); }
  return session_->log_data_[index_].stamp;
}

ros::Time Log::relativeTime() const
{
  if (!session_) { return ros::Time(0); }
  ros::Duration delta = session_->log_data_[index_].stamp - session_->min_time_;

  ros::Time rel_time;
  rel_time.sec = delta.sec;
  rel_time.nsec = delta.nsec;  
  return rel_time;  
}

uint8_t Log::severity() const
{
  if (!session_) { return 0xFF; }
  return session_->log_data_[index_].severity;
}

int Log::nodeId() const
{
  if (!session_) { return -1; }
  return session_->log_data_[index_].node_id;
}

QString Log::nodeName() const
{
  if (!session_) { return "invalid log"; }
  return session_->db_->nodeName(nodeId());
}

QString Log::functionName() const
{
  if (!session_) { return QString(); }
  return session_->log_data_[index_].function;
}

QString Log::fileName() const
{
  if (!session_) { return QString(); }
  return session_->log_data_[index_].file;
}

uint32_t Log::lineNumber() const
{
  if (!session_) { return 0; }
  return session_->log_data_[index_].line;
}

QStringList Log::textLines() const
{
  if (!session_) { return QStringList(); }
  return session_->log_data_[index_].text_lines;
}

QString Log::textSingleLine() const
{
  return textLines().join(" ");
}
}  // namespace swri_console
