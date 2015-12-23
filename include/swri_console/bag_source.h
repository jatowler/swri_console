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
#ifndef SWRI_CONSOLE_BAG_SOURCE_H_
#define SWRI_CONSOLE_BAG_SOURCE_H_

#include <QObject>
#include <QThread>
#include <rosgraph_msgs/Log.h>

namespace swri_console
{
class BagSourceBackend;

class BagSource : public QObject
{
  Q_OBJECT;

 public:
  BagSource(const QString &filename);
  ~BagSource();

  void start();
  
  const QString &filename() const { return filename_; }
  

 Q_SIGNALS:
  void finished(const QString &name, bool success, size_t msg_count, const QString &error_msg);
  void logRead(const rosgraph_msgs::LogConstPtr &msg);

 private Q_SLOTS:
  void handleFinished(bool success, size_t msg_count, QString error_msg);
  void handleLogRead(const rosgraph_msgs::LogConstPtr &msg);

 private:
  const QString filename_;  
  BagSourceBackend *backend_;
  QThread thread_;
};  // class BagSource
}  // namespace swri_console
#endif  // SWRI_CONSOLE_BAG_SOURCE_H_

