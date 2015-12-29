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
#include <swri_console/log_filter.h>
#include <swri_console/log.h>

#include <QDebug>

namespace swri_console
{
LogFilter::LogFilter(QObject *parent)
  :
  QObject(parent),
  severity_mask_(0xFF)
{
}

bool LogFilter::accept(const Log& log) const
{
  if (!(log.severity() & severity_mask_)) {
    return false;
  }

  if (nids_.count(log.nodeId()) == 0) {
    return false;
  }

  QString text = log.textSingleLine();

  if (include_regexp_.indexIn(text) < 0) {
    return false;
  }

  if (!exclude_regexp_.isEmpty() && exclude_regexp_.indexIn(text) >= 0) {
    return false;
  }
  
  return true;
}

void LogFilter::setNodeFilter(const std::vector<int> &nids)
{
  nids_.clear();
  nids_.insert(nids.begin(), nids.end());
  Q_EMIT filterModified();
}

void LogFilter::setSeverityMask(uint8_t mask)
{
  if (severity_mask_ == mask) { return; }
  severity_mask_ = mask;
  Q_EMIT filterModified();
}

void LogFilter::setIncludeRegExp(const QRegExp &regexp)
{
  if (include_regexp_ == regexp) { return; }
  include_regexp_ = regexp;
  Q_EMIT filterModified();
}

void LogFilter::setExcludeRegExp(const QRegExp &regexp)
{
  if (exclude_regexp_ == regexp) { return; }
  exclude_regexp_ = regexp;
  Q_EMIT filterModified();
}
}  // namespace swri_console
