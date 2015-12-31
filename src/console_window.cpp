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

#include <stdint.h>
#include <stdio.h>
#include <set>

#include <swri_console/console_window.h>
#include <swri_console/log_database.h>
#include <swri_console/log_filter.h>
#include <swri_console/settings_keys.h>

#include <QApplication>
#include <QClipboard>
#include <QColorDialog>
#include <QDateTime>
#include <QDebug>
#include <QDir>
#include <QFileDialog>
#include <QFontDialog>
#include <QMenu>
#include <QRegExp>
#include <QScrollBar>
#include <QSettings>

using namespace Qt;

namespace swri_console {

ConsoleWindow::ConsoleWindow(LogDatabase *db)
  :
  QMainWindow(),
  db_(db)
{
  ui.setupUi(this); 

  ui.sessionList->setDatabase(db);
  ui.nodeList->setDatabase(db);
  ui.logList->setDatabase(db);

  ui.sessionList->setContextMenuPolicy(Qt::ActionsContextMenu);
  ui.sessionList->addAction(ui.action_ForceNewLiveSession);
  
  ui.logList->setContextMenuPolicy(Qt::ActionsContextMenu);
  ui.logList->addAction(ui.action_SelectAll);
  ui.logList->addAction(ui.action_Copy);
  ui.logList->addAction(ui.action_CopyExtended);

  // Connect the session selection to the node list (used to filter
  // message counts).
  QObject::connect(ui.sessionList,
                   SIGNAL(selectionChanged(const std::vector<int>&)),
                   ui.nodeList,
                   SLOT(setSessionFilter(const std::vector<int>&)));

  // Connect the session selection to the log list (used to filter
  // logs).
  QObject::connect(ui.sessionList,
                   SIGNAL(selectionChanged(const std::vector<int>&)),
                   ui.logList,
                   SLOT(setSessionFilter(const std::vector<int>&)));

  // Connect the node selection to the log list's filter proxy.
  QObject::connect(ui.nodeList,
                   SIGNAL(selectionChanged(const std::vector<int>&)),
                   ui.logList->logFilter(),
                   SLOT(setNodeFilter(const std::vector<int>&)));

  // Connect the node selection to ourselves so we can change the
  // window title.
  QObject::connect(ui.nodeList,
                   SIGNAL(selectionChanged(const std::vector<int>&)),
                   this,
                   SLOT(nodeSelectionChanged(const std::vector<int>&)));
    
  QObject::connect(ui.action_NewWindow, SIGNAL(triggered(bool)),
                   this, SIGNAL(createNewWindow()));
  QObject::connect(ui.action_ForceNewLiveSession, SIGNAL(triggered()),
                   this, SIGNAL(forceNewLiveSession()));

  QObject::connect(ui.action_SelectAll, SIGNAL(triggered()),
                   ui.logList, SLOT(selectAll()));
  QObject::connect(ui.action_Copy, SIGNAL(triggered()),
                   ui.logList, SLOT(copyLogsToClipboard()));
  QObject::connect(ui.action_CopyExtended, SIGNAL(triggered()),
                   ui.logList, SLOT(copyExtendedLogsToClipboard()));


  QObject::connect(ui.action_ReadBagFile, SIGNAL(triggered(bool)),
                   this, SLOT(promptForBagFile()));

  QObject::connect(ui.action_SaveLogs, SIGNAL(triggered(bool)),
                   this, SLOT(saveLogs()));

  timestamp_actions_ = new QActionGroup(this);
  timestamp_actions_->setExclusive(true);
  timestamp_actions_->addAction(ui.action_NoTimestamps);
  timestamp_actions_->addAction(ui.action_RelativeTimestamps);
  timestamp_actions_->addAction(ui.action_AbsoluteTimestamps);  
  QObject::connect(timestamp_actions_, SIGNAL(triggered(QAction *)),
                   this, SLOT(handleTimestampActions()));

  QObject::connect(ui.action_SelectFont, SIGNAL(triggered(bool)),
                   this, SLOT(selectFont()));

  QObject::connect(ui.debugColor, SIGNAL(colorEdited(const QColor &)),
                   ui.logList, SLOT(setDebugColor(const QColor &)));
  QObject::connect(ui.infoColor, SIGNAL(colorEdited(const QColor &)),
                   ui.logList, SLOT(setInfoColor(const QColor &)));
  QObject::connect(ui.warnColor, SIGNAL(colorEdited(const QColor &)),
                   ui.logList, SLOT(setWarnColor(const QColor &)));
  QObject::connect(ui.errorColor, SIGNAL(colorEdited(const QColor &)),
                   ui.logList, SLOT(setErrorColor(const QColor &)));
  QObject::connect(ui.fatalColor, SIGNAL(colorEdited(const QColor &)),
                   ui.logList, SLOT(setFatalColor(const QColor &)));

  QObject::connect(ui.checkDebug, SIGNAL(toggled(bool)),
                   ui.logList->logFilter(), SLOT(enableDebug(bool)));
  QObject::connect(ui.checkInfo, SIGNAL(toggled(bool)),
                   ui.logList->logFilter(), SLOT(enableInfo(bool)));
  QObject::connect(ui.checkWarn, SIGNAL(toggled(bool)),
                   ui.logList->logFilter(), SLOT(enableWarn(bool)));
  QObject::connect(ui.checkError, SIGNAL(toggled(bool)),
                   ui.logList->logFilter(), SLOT(enableError(bool)));
  QObject::connect(ui.checkFatal, SIGNAL(toggled(bool)),
                   ui.logList->logFilter(), SLOT(enableFatal(bool)));

  ui.checkFollowNewest->setChecked(true);
  ui.logList->setAutoScrollToBottom(true);
  QObject::connect(ui.checkFollowNewest, SIGNAL(toggled(bool)), 
                   ui.logList, SLOT(setAutoScrollToBottom(bool)));
  QObject::connect(ui.logList, SIGNAL(autoScrollToBottomChanged(bool)),
                   ui.checkFollowNewest, SLOT(setChecked(bool)));

  QObject::connect(ui.clearAllButton, SIGNAL(clicked()),
                    this, SLOT(resetDatabase()));

  // With the new session model, the clear messages operation
  // corresponds to either deleting all session (from data
  // perspective) or clearing the session selection (from user's
  // perspective).  We're going with the latter to prevent losing
  // useful previous session data.  Users can delete sessions through
  // the session list context menu.
  QObject::connect(ui.clearMessagesButton, SIGNAL(clicked()),
                   ui.sessionList, SLOT(deselectAll()));

  QObject::connect(ui.includeText, SIGNAL(textEdited(const QString &)),
                   this, SLOT(processFilterText()));
  QObject::connect(ui.excludeText, SIGNAL(textEdited(const QString &)),
                   this, SLOT(processFilterText()));
  QObject::connect(ui.action_RegularExpressions, SIGNAL(toggled(bool)),
                   this, SLOT(processFilterText()));
 
  QList<int> sizes;
  sizes.append(100);
  sizes.append(1000);
  ui.horizontalSplitter->setSizes(sizes);
  ui.verticalSplitter->setSizes(sizes);

  statusBar()->setSizeGripEnabled(false);
  connection_status_ = new QLabel("Not connected");
  connection_status_->setFrameStyle(QFrame::Panel | QFrame::Sunken);
  statusBar()->addPermanentWidget(connection_status_);
  
  loadSettings();
}

ConsoleWindow::~ConsoleWindow()
{
}

void ConsoleWindow::resetDatabase()
{
  db_->clear();
}

void ConsoleWindow::saveLogs()
{
  QString defaultname = QDateTime::currentDateTime().toString(Qt::ISODate) + ".bag";
  QString filename = QFileDialog::getSaveFileName(this,
                                                  "Save Logs",
                                                  QDir::homePath() + QDir::separator() + defaultname,
                                                  tr("Bag Files (*.bag);;Text Files (*.txt)"));
  if (filename != NULL && !filename.isEmpty()) {
    // db_proxy_->saveToFile(filename);
  }
}

void ConsoleWindow::rosConnected(bool connected, const QString &master_uri)
{
  if (connected) {    
    connection_status_->setText(master_uri);
  } else {
    connection_status_->setText("Not connected");
  }
}

void ConsoleWindow::closeEvent(QCloseEvent *event)
{
  saveSettings();
  QMainWindow::closeEvent(event);
}

void ConsoleWindow::nodeSelectionChanged(const std::vector<int>& nids)
{
  QStringList node_names;
  for (auto nid : nids) {
    QString full_name = db_->nodeName(nid);
    QString node_name = full_name.split("/", QString::SkipEmptyParts).last();
    node_names.append(node_name);
  }

  if (node_names.size() == 0) {
    setWindowTitle("SwRI Console");
  } else if (node_names.size() > 5) {
    setWindowTitle(QString("SwRI Console (%1 nodes)").arg(node_names.size()));
  } else {
    setWindowTitle(QString("SwRI Console (") + node_names.join(", ") + ")");
  }
}

void ConsoleWindow::selectFont()
{
  QFont starting_font = data_font_;

  QFontDialog dlg(data_font_);
    
  QObject::connect(&dlg, SIGNAL(currentFontChanged(const QFont &)),
                   this, SLOT(setFont(const QFont &)));

  int ret = dlg.exec();

  if (ret == QDialog::Accepted) {
    setFont(dlg.selectedFont());
  } else {
    setFont(starting_font);
  }
}

void ConsoleWindow::setFont(const QFont &font)
{
  data_font_ = font;
  ui.includeText->setFont(data_font_);
  ui.excludeText->setFont(data_font_);
  ui.sessionList->setFont(data_font_);
  ui.nodeList->setFont(data_font_);
  ui.logList->setFont(data_font_);
}

void ConsoleWindow::loadSettings()
{
  QSettings settings;

  {
    QFont font = QFont("Ubuntu Mono", 9);
    font = settings.value(SettingsKeys::FONT, font).value<QFont>();
    setFont(font);
  }
  
  {
    int format = settings.value(SettingsKeys::TIMESTAMP_FORMAT, STAMP_FORMAT_RELATIVE).toInt();
    ui.action_NoTimestamps->setChecked(false);
    ui.action_RelativeTimestamps->setChecked(false);
    ui.action_AbsoluteTimestamps->setChecked(false);    
    if (format == STAMP_FORMAT_NONE) {
      ui.action_NoTimestamps->setChecked(true);
    } else if (format == STAMP_FORMAT_RELATIVE) {
      ui.action_RelativeTimestamps->setChecked(true);
    } else if (format == STAMP_FORMAT_ABSOLUTE) {
      ui.action_AbsoluteTimestamps->setChecked(true);
    }    
    ui.logList->setStampFormat(selectedStampFormat());
  }
  
  { // Load severity masks
    bool enabled;
    enabled = settings.value(SettingsKeys::SHOW_DEBUG, true).toBool();
    ui.checkDebug->setChecked(enabled);
    ui.logList->logFilter()->enableDebug(enabled);

    enabled = settings.value(SettingsKeys::SHOW_INFO, true).toBool();
    ui.checkInfo->setChecked(enabled);
    ui.logList->logFilter()->enableInfo(enabled);

    enabled = settings.value(SettingsKeys::SHOW_WARN, true).toBool();
    ui.checkWarn->setChecked(enabled);
    ui.logList->logFilter()->enableWarn(enabled);

    enabled = settings.value(SettingsKeys::SHOW_ERROR, true).toBool();
    ui.checkError->setChecked(enabled);
    ui.logList->logFilter()->enableError(enabled);

    enabled = settings.value(SettingsKeys::SHOW_FATAL, true).toBool();
    ui.checkFatal->setChecked(enabled);
    ui.logList->logFilter()->enableFatal(enabled);
  }
  
  { // Load button colors.
    QColor color;
    color = settings.value(SettingsKeys::DEBUG_COLOR, Qt::gray).value<QColor>();
    ui.debugColor->setColor(color);
    ui.logList->setDebugColor(color);

    color = settings.value(SettingsKeys::INFO_COLOR, Qt::black).value<QColor>();
    ui.infoColor->setColor(color);
    ui.logList->setInfoColor(color);

    color = settings.value(SettingsKeys::WARN_COLOR, QColor(255, 127, 0)).value<QColor>();
    ui.warnColor->setColor(color);
    ui.logList->setWarnColor(color);

    color = settings.value(SettingsKeys::ERROR_COLOR, Qt::red).value<QColor>();
    ui.errorColor->setColor(color);
    ui.logList->setErrorColor(color);

    color = settings.value(SettingsKeys::FATAL_COLOR, Qt::magenta).value<QColor>();
    ui.fatalColor->setColor(color);
    ui.logList->setFatalColor(color);
  }

  // Finally, load the filter contents.
  loadBooleanSetting(SettingsKeys::USE_REGEXPS, ui.action_RegularExpressions);
  QString includeFilter = settings.value(SettingsKeys::INCLUDE_FILTER, "").toString();
  ui.includeText->setText(includeFilter);
  QString excludeFilter = settings.value(SettingsKeys::EXCLUDE_FILTER, "").toString();
  ui.excludeText->setText(excludeFilter);
  processFilterText();
}

void ConsoleWindow::saveSettings()
{
  QSettings settings;
  settings.setValue(SettingsKeys::FONT, data_font_);

  settings.setValue(SettingsKeys::TIMESTAMP_FORMAT, selectedStampFormat());
  
  settings.setValue(SettingsKeys::USE_REGEXPS, ui.action_RegularExpressions->isChecked());
  settings.setValue(SettingsKeys::INCLUDE_FILTER, ui.includeText->text());
  settings.setValue(SettingsKeys::EXCLUDE_FILTER, ui.excludeText->text());

  settings.setValue(SettingsKeys::DEBUG_COLOR, ui.debugColor->color());
  settings.setValue(SettingsKeys::INFO_COLOR, ui.infoColor->color());
  settings.setValue(SettingsKeys::WARN_COLOR, ui.warnColor->color());
  settings.setValue(SettingsKeys::ERROR_COLOR, ui.errorColor->color());
  settings.setValue(SettingsKeys::FATAL_COLOR, ui.fatalColor->color());

  settings.setValue(SettingsKeys::SHOW_DEBUG, ui.checkDebug->isChecked());
  settings.setValue(SettingsKeys::SHOW_INFO, ui.checkInfo->isChecked());
  settings.setValue(SettingsKeys::SHOW_WARN, ui.checkWarn->isChecked());
  settings.setValue(SettingsKeys::SHOW_ERROR, ui.checkError->isChecked());
  settings.setValue(SettingsKeys::SHOW_FATAL, ui.checkFatal->isChecked());
}

void ConsoleWindow::promptForBagFile()
{
  QStringList filenames = QFileDialog::getOpenFileNames(
    NULL,
    tr("Open Bag File(s)"),
    QDir::homePath(),
    tr("Bag Files (*.bag)"));

  for (int i = 0; i < filenames.size(); i++) {
    Q_EMIT readBagFile(filenames[i]);
  }
}

static
QRegExp regExpFromText(const QString &text, bool use_regular_expression)
{
  if (use_regular_expression) {
    return QRegExp(text, Qt::CaseInsensitive);
  } else {
    QStringList raw_items = text.split(";", QString::SkipEmptyParts);
    QStringList esc_items;
    
    for (int i = 0; i < raw_items.size(); i++) {
      esc_items.append(QRegExp::escape(raw_items[i].trimmed()));
    }

    QString pattern = esc_items.join("|");
    return QRegExp(pattern, Qt::CaseInsensitive);
  }
}

void ConsoleWindow::processFilterText()
{
  bool use_re = ui.action_RegularExpressions->isChecked();

  QRegExp re = regExpFromText(ui.includeText->text(), use_re);
  if (re.isValid()) {
    ui.includeLabel->setStyleSheet("QLabel { }");
    ui.logList->logFilter()->setIncludeRegExp(re);
  } else {
    ui.includeLabel->setStyleSheet("QLabel { background-color : red; color : white; }");
  }

  re = regExpFromText(ui.excludeText->text(), use_re);
  if (re.isValid()) {
    ui.excludeLabel->setStyleSheet("QLabel { }");
    ui.logList->logFilter()->setExcludeRegExp(re);
  } else {
    ui.excludeLabel->setStyleSheet("QLabel { background-color : red; color : white; }");
  }  
}

void ConsoleWindow::handleTimestampActions()
{
  ui.logList->setStampFormat(selectedStampFormat());
}

StampFormat ConsoleWindow::selectedStampFormat() const
{
  if (ui.action_NoTimestamps->isChecked()) {
    return STAMP_FORMAT_NONE;
  } else if (ui.action_RelativeTimestamps->isChecked()) {
    return STAMP_FORMAT_RELATIVE;
  } else if (ui.action_AbsoluteTimestamps->isChecked()) {
    return STAMP_FORMAT_ABSOLUTE;
  } else {
    qWarning("Unexpected state in %s", __PRETTY_FUNCTION__);
    return STAMP_FORMAT_RELATIVE;
  }    
}
}  // namespace swri_console

