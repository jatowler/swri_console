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
#include <swri_console/ros_source.h>

#include <QDateTime>
#include <QTimer>

#include <swri_console/ros_source_backend.h>
#include <swri_console/log_database.h>

namespace swri_console
{
RosSource::RosSource(LogDatabase *db)
  :
  backend_(NULL),
  connected_(false),
  db_(db),
  session_id_(-1)
{
  QObject::connect(db_, SIGNAL(sessionDeleted(int)),
                   this, SLOT(handleSessionDeleted(int)));
  QObject::connect(db_, SIGNAL(databaseCleared()),
                   this, SLOT(resetSessionId()));
}

RosSource::~RosSource()
{
  ros_thread_.quit();
  if (!ros_thread_.wait(500)) {
    qWarning("ROS thread is not closing in a timely fashion.  This seems to "
             "happen with the network connection is lost or ROS master has "
             "shutdown.  We will attempt to forcibly terminate the thread.");
    ros_thread_.terminate();
  }
}

void RosSource::start()
{
  if (backend_) {
    return;
  }

  // Using the threading approach recommended in the following URL.
  // https://mayaposch.wordpress.com/2011/11/01/how-to-really-truly-use-qthreads-the-full-explanation/
  backend_ = new RosSourceBackend();
  backend_->moveToThread(&ros_thread_);

  // The backend should delete itself once the thread has finished.
  QObject::connect(&ros_thread_, SIGNAL(finished()),
                   backend_, SLOT(deleteLater()));

  QObject::connect(backend_, SIGNAL(connected(bool, QString)),
                   this, SLOT(handleConnected(bool, QString)));
  QObject::connect(backend_, SIGNAL(logReceived(const rosgraph_msgs::LogConstPtr &)),
                   this, SLOT(handleLog(const rosgraph_msgs::LogConstPtr &)));
  ros_thread_.start();
}

void RosSource::handleConnected(bool is_connected, QString uri)
{  
  connected_ = is_connected;
  master_uri_ = uri;

  if (connected_) {
    createNewSession();
  } else {
    session_id_ = -1;
  }
  
  Q_EMIT connected(connected_, master_uri_);
}

void RosSource::handleLog(const rosgraph_msgs::LogConstPtr &msg)
{
  if (session_id_ < 0) {
    createNewSession();
  }

  Session *session = &(db_->session(session_id_));
  
  if (!session->isValid()) {
    session_id_ = -1;
    createNewSession();
    session = &(db_->session(session_id_));
  }

  session->append(msg);
}

void RosSource::createNewSession()
{
  // Only create a new session if the current one is invalid, this is
  // to prevent the situation where we schedule a new session after a
  // database reset, then handle a message, and then service the timer
  // callback, which would create two sessions instead of one.
  if (connected_ && session_id_ < 0) {
    QDateTime now = QDateTime::currentDateTime();
    session_id_ = db_->createSession(QString("Live at %1").arg(now.toString("hh:mm:ss")));
    Q_EMIT liveSessionChanged(session_id_);
  }
}

void RosSource::handleSessionDeleted(int sid)
{
  if (sid == session_id_) {
    resetSessionId();
  }
}

void RosSource::resetSessionId()
{
  session_id_ = -1;
  // We don't want to create a new session during a reset because
  // other objects are also handling the reset and expecting the
  // database to be completely cleared out, so we schedule a timer
  // to create the session right after this.
  QTimer::singleShot(0, this, SLOT(createNewSession()));
}
}  // namespace swri_console
