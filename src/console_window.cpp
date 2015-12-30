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

#include <QColorDialog>
#include <QRegExp>
#include <QApplication>
#include <QClipboard>
#include <QDateTime>
#include <QFileDialog>
#include <QDir>
#include <QScrollBar>
#include <QMenu>
#include <QSettings>
#include <QDebug>

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

  // QObject::connect(ui.action_AbsoluteTimestamps, SIGNAL(toggled(bool)),
  //                  db_proxy_, SLOT(setAbsoluteTime(bool)));

  // QObject::connect(ui.action_ShowTimestamps, SIGNAL(toggled(bool)),
  //                  db_proxy_, SLOT(setDisplayTime(bool)));

  QObject::connect(ui.action_SelectFont, SIGNAL(triggered(bool)),
                   this, SIGNAL(selectFont()));

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

  // Right-click menu for the message list
  // QObject::connect(ui.messageList, SIGNAL(customContextMenuRequested(const QPoint&)),
  //                   this, SLOT(showLogContextMenu(const QPoint&)));

  QObject::connect(ui.clearAllButton, SIGNAL(clicked()),
                    this, SLOT(clearAll()));
  QObject::connect(ui.clearMessagesButton, SIGNAL(clicked()),
                    this, SLOT(clearMessages()));

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

void ConsoleWindow::clearAll()
{
  db_->clear();
  //node_list_model_->clear();
}

void ConsoleWindow::clearMessages()
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

void ConsoleWindow::setFont(const QFont &font)
{
  // ui.messageList->setFont(font);
  // ui.nodeList->setFont(font);
}

void ConsoleWindow::loadSettings()
{
  // First, load all the boolean settings...
  loadBooleanSetting(SettingsKeys::DISPLAY_TIMESTAMPS, ui.action_ShowTimestamps);
  loadBooleanSetting(SettingsKeys::ABSOLUTE_TIMESTAMPS, ui.action_AbsoluteTimestamps);

  // The severity level has to be handled a little differently, since they're all combined
  // into a single integer mask under the hood.  First they have to be loaded from the settings,
  // then set in the UI, then the mask has to actually be applied.
  QSettings settings;

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

    color = settings.value(SettingsKeys::INFO_COLOR, Qt::gray).value<QColor>();
    ui.infoColor->setColor(color);
    ui.logList->setInfoColor(color);

    color = settings.value(SettingsKeys::WARN_COLOR, Qt::gray).value<QColor>();
    ui.warnColor->setColor(color);
    ui.logList->setWarnColor(color);

    color = settings.value(SettingsKeys::ERROR_COLOR, Qt::gray).value<QColor>();
    ui.errorColor->setColor(color);
    ui.logList->setErrorColor(color);

    color = settings.value(SettingsKeys::FATAL_COLOR, Qt::gray).value<QColor>();
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

  settings.setValue(SettingsKeys::USE_REGEXPS, ui.action_RegularExpressions->isChecked());
  settings.setValue(SettingsKeys::INCLUDE_FILTER, ui.includeText->text());
  settings.setValue(SettingsKeys::EXCLUDE_FILTER, ui.excludeText->text());
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
}  // namespace swri_console

