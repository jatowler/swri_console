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
#include <swri_console/bag_source.h>
#include <swri_console/bag_source_backend.h>

namespace swri_console
{
BagSource::BagSource(const QString &filename)
  :
  filename_(filename),
  backend_(NULL)
{
}

BagSource::~BagSource()
{
  thread_.quit();
  if (!thread_.wait(500)) {
    qWarning("Bag thread is not closing in a timely fashion.  This can happen"
             "when opening a really large file.  We will attempt to forcibly "
             "terminate the thread.");
    thread_.terminate();
  }
}

void BagSource::start()
{
  if (backend_) {
    return;
  }

  // Using the threading approach recommended in the following URL.
  // https://mayaposch.wordpress.com/2011/11/01/how-to-really-truly-use-qthreads-the-full-explanation/
  backend_ = new BagSourceBackend(filename_);
  backend_->moveToThread(&thread_);
  // The thread should finish when the backend has finished.
  QObject::connect(backend_, SIGNAL(finished(bool, size_t, QString)),
                   &thread_, SLOT(quit()));
  // The backend should delete itself once the thread has finished.
  QObject::connect(&thread_, SIGNAL(finished()),
                   backend_, SLOT(deleteLater()));

  QObject::connect(backend_, SIGNAL(finished(bool, size_t, QString)),
                   this, SLOT(handleFinished(bool, size_t, QString)));
  QObject::connect(backend_, SIGNAL(logRead(const rosgraph_msgs::LogConstPtr &)),
                   this, SLOT(handleLogRead(const rosgraph_msgs::LogConstPtr &)));
  thread_.start();
}

void BagSource::handleFinished(bool success, size_t msg_count, QString error_msg)
{
  Q_EMIT finished(filename_, success, msg_count, error_msg);
}

void BagSource::handleLogRead(const rosgraph_msgs::LogConstPtr &msg)
{
  // Planning on changing what is done here soon, so that's why we're
  // just rebroadcasting the signal from a slot instead of linking the
  // incoming signal to the outgoing signal.
  Q_EMIT logRead(msg);
}                          
}  // namespace swri_console
