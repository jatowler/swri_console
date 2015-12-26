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

#include <swri_console/console_master.h>
#include <swri_console/console_window.h>
#include <swri_console/settings_keys.h>
#include <swri_console/bag_source.h>

#include <QFontDialog>
#include <QSettings>

namespace swri_console
{
ConsoleMaster::ConsoleMaster(int argc, char** argv)
  :
  window_font_(QFont("Ubuntu Mono", 9)),
  ros_source_(&db_),
  connected_(false)
{
  ros_source_.start();
}

ConsoleMaster::~ConsoleMaster()
{
}

void ConsoleMaster::createNewWindow()
{
  ConsoleWindow* win = new ConsoleWindow(&db_);

  QSettings settings;
  window_font_ = settings.value(SettingsKeys::FONT, QFont("Ubuntu Mono", 9)).value<QFont>();
  win->setFont(window_font_);
  QObject::connect(win, SIGNAL(createNewWindow()),
                   this, SLOT(createNewWindow()));

  QObject::connect(&ros_source_, SIGNAL(connected(bool, const QString&)),
                   win, SLOT(rosConnected(bool, const QString&)));
  win->rosConnected(ros_source_.isConnected(), ros_source_.masterUri());

  QObject::connect(this,
                   SIGNAL(fontChanged(const QFont &)),
                   win, SLOT(setFont(const QFont &)));

  QObject::connect(win, SIGNAL(selectFont()),
                   this, SLOT(selectFont()));

  QObject::connect(win, SIGNAL(readBagFile(const QString &)),
                   this, SLOT(readBagFile(const QString &)));

  win->show();
}

void ConsoleMaster::fontSelectionChanged(const QFont &font)
{
  window_font_ = font;
  QSettings settings;
  settings.setValue(SettingsKeys::FONT, font);
  Q_EMIT fontChanged(window_font_);
}

void ConsoleMaster::selectFont()
{
  QFont starting_font = window_font_;

  QFontDialog dlg(window_font_);
    
  QObject::connect(&dlg, SIGNAL(currentFontChanged(const QFont &)),
                   this, SLOT(fontSelectionChanged(const QFont &)));

  int ret = dlg.exec();

  if (ret == QDialog::Accepted) {
    if (window_font_ != dlg.selectedFont()) {
      window_font_ = dlg.selectedFont();
      Q_EMIT fontChanged(window_font_);
    }
  } else {
    if (window_font_ != starting_font) {
      window_font_ = starting_font;
      Q_EMIT fontChanged(window_font_);
    }
  }
}

void ConsoleMaster::readBagFile(const QString &name)
{
  BagSource *source = new BagSource(&db_, name);
  
  QObject::connect(source, SIGNAL(finished(const QString&, bool, size_t, const QString&)),
                   source, SLOT(deleteLater()));

  // NOTE: There is currently a risk of segfault and/or temporary
  // hanging if program is closed before the bag source is finished.
  // Need to add a mechanism to abort the bag source when the last
  // window is closed.
  
  source->start();
}
}  // namespace swri_console
