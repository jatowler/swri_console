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
#ifndef SWRI_CONSOLE_BAG_SOURCE_BACKEND_H_
#define SWRI_CONSOLE_BAG_SOURCE_BACKEND_H_

#include <QObject>
#include <rosgraph_msgs/Log.h>
#include <rosbag/bag.h>
#include <rosbag/view.h>

namespace swri_console
{
class BagSourceBackend : public QObject
{
  Q_OBJECT;
  
 public:
  BagSourceBackend(const QString &filename);
  ~BagSourceBackend();
  
 Q_SIGNALS:
  void finished(bool success, size_t msg_count, QString error_msg);
  void logRead(const rosgraph_msgs::LogConstPtr &msg);

 protected:
  void timerEvent(QTimerEvent *);

 private:
  enum ResultStatus {
    CONTINUE,
    FINISHED,
    ERROR
  };
    
  struct Result {
    ResultStatus status;
    QString error_msg;

    Result() : status(CONTINUE) {}
    Result(ResultStatus status, const QString &error_msg="")
      : status(status), error_msg(error_msg) {}
  };
  
  Result open();
  Result read(size_t msgs_to_read);
  void close();
  
 private:
  const QString filename_;
  int timer_id_;
  bool opened_;
  rosbag::Bag bag_;
  rosbag::View *view_;
  rosbag::View::const_iterator iter_;
  size_t msg_count_;
};
}  // namespace swri_console
#endif  // SWRI_CONSOLE_BAG_SOURCE_BACKEND_H_
