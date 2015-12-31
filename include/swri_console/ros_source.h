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
#ifndef SWRI_CONSOLE_ROS_SOURCE_H_
#define SWRI_CONSOLE_ROS_SOURCE_H_

#include <QObject>
#include <QThread>
#include <rosgraph_msgs/Log.h>

namespace swri_console
{
class LogDatabase;
class RosSourceBackend;

/* RosSource is the interface class for interacting with ROS.  It runs
 * in the GUI thread and hides all the actual interactions with ROS in
 * a different thread in the RosSourceBackend.
 */
class RosSource : public QObject
{
  Q_OBJECT;

 public:
  /*
   * Creates a new ROS source.  The source will not become active
   * until start is called.  This gives other components a chance to
   * setup signal/slot connections before anything can change behind
   * the scenes.
   */
  RosSource(LogDatabase *db);

  /*
   * Destroys the ROS source.  Will call shutdown() if the source is
   * currently running.
   */
  ~RosSource();

  /*
   * Return the connection status.
   */
  bool isConnected() const { return connected_; }

  /*
   * Return the current ROS master URI if the source is connected.  If
   * not connected, returns an empty string.
   */
  const QString& masterUri() const { return master_uri_; }

  /*
   * Called to start the source.  If the source has already been
   * started, this will do nothing.  I'm not sure if it safe to
   * stop/restart the thread, and we don't have a current use case
   * that needs it. (Only possible feature that might need it is to
   * inhibit connecting to the ROS master automatically).
   */
  void start();


 Q_SIGNALS:
  /**
   * Emitted every time we are connect to or disconnect from ROS.
   */
  void connected(bool connected, const QString &mater_uri);

  /**
   * Emitted when the source creates a new capture session.
   */
  void liveSessionChanged(int session_id);

 public Q_SLOTS:
  void handleSessionDeleted(int sid);

 private Q_SLOTS:
  // Used internally to catch when the ROS source backend connects or
  // disconnects.  This connection is between two different threads
  // and so will be queued.
  void handleConnected(bool connected, QString uri);

  // Used internally to receive log messages from the ROS source
  // backend.  This connection is between two different threads and so
  // will be queued.
  void handleLog(const rosgraph_msgs::LogConstPtr &msg);

  void createNewSession();
  
 private:
  QThread ros_thread_;
  RosSourceBackend *backend_;

  bool connected_;
  QString master_uri_;

  LogDatabase *db_;
  int session_id_;
};  // class RosSource
}  // namespace swri_console
#endif //SWRI_CONSOLE_ROS_SOURCE_H_
