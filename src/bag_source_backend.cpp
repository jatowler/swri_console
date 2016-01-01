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
#include <swri_console/bag_source_backend.h>

namespace swri_console
{
// Number of messages to read from the bag file during each operation.
static const int CHUNK_SIZE = 200;

BagSourceBackend::BagSourceBackend(const QString &filename)
  :
  filename_(filename),
  opened_(false),
  view_(NULL),
  msg_count_(0)
{
  timer_id_ = startTimer(0);
}

BagSourceBackend::~BagSourceBackend()
{
  if (view_) {
    delete view_;
  }  
}

void BagSourceBackend::timerEvent(QTimerEvent *)
{
  Result result;

  try {
    if (!opened_) {
      result = open();
    } else {
      result = read(CHUNK_SIZE);
    }
  } catch(const rosbag::BagException &e) {
    result = Result(ERROR, QString("Bag file error: %1").arg(e.what()));
  }

  if (result.status != CONTINUE) {
    killTimer(timer_id_);
    if (view_) {
      delete view_;
      view_ = NULL;
    }
    bag_.close();
    Q_EMIT finished(result.status == FINISHED, msg_count_, result.error_msg);
  }
}

BagSourceBackend::Result BagSourceBackend::open()
{
  bool has_rosout = false;
  bool has_rosout_agg = false;
  
  bag_.open(filename_.toStdString(), rosbag::bagmode::Read);
    
  has_rosout_agg = !(rosbag::View(bag_, rosbag::TopicQuery("/rosout_agg")).getConnections().empty());
  if (!has_rosout_agg) {
    has_rosout = !(rosbag::View(bag_, rosbag::TopicQuery("/rosout")).getConnections().empty());
  }

  if (!has_rosout && !has_rosout_agg) {
    return Result(ERROR, QString("Bag file does not have /rosout or /rosout_agg"));
  }

  std::string topic = has_rosout_agg ? "/rosout_agg" : "/rosout";

  // View has a private assignment operator, so the only way to create
  // a persistent view after our constructor is to allocate it on the
  // heap.
  view_ = new rosbag::View(bag_, rosbag::TopicQuery(topic));
  iter_ = view_->begin();

  opened_ = true;  
  return Result(CONTINUE);
}

BagSourceBackend::Result BagSourceBackend::read(size_t msgs_to_read)
{
  for (size_t i = 0; i < msgs_to_read; ++i) {
    if (iter_ == view_->end()) {
      return Result(FINISHED, "");
    }
    rosgraph_msgs::LogConstPtr log = iter_->instantiate<rosgraph_msgs::Log>();
    if (log != NULL ) {
      emit logRead(log);
      msg_count_++;
    } else {
      qWarning("Got a message that was not a log message but a: %s", iter_->getDataType().c_str());
    }
    
    ++iter_;
  }

  return Result(CONTINUE);
}
}  // namespace swri_console

