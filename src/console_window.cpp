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

#include <rosgraph_msgs/Log.h>

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

  // QObject::connect(ui.action_ColorizeLogs, SIGNAL(toggled(bool)),
  //                  db_proxy_, SLOT(setColorizeLogs(bool)));

  QObject::connect(ui.debugColorWidget, SIGNAL(clicked(bool)),
                   this, SLOT(setDebugColor()));
  QObject::connect(ui.infoColorWidget, SIGNAL(clicked(bool)),
                   this, SLOT(setInfoColor()));
  QObject::connect(ui.warnColorWidget, SIGNAL(clicked(bool)),
                   this, SLOT(setWarnColor()));
  QObject::connect(ui.errorColorWidget, SIGNAL(clicked(bool)),
                   this, SLOT(setErrorColor()));
  QObject::connect(ui.fatalColorWidget, SIGNAL(clicked(bool)),
                   this, SLOT(setFatalColor()));

  QObject::connect(ui.checkDebug, SIGNAL(toggled(bool)),
                   this, SLOT(setSeverityFilter()));
  QObject::connect(ui.checkInfo, SIGNAL(toggled(bool)),
                   this, SLOT(setSeverityFilter()));
  QObject::connect(ui.checkWarn, SIGNAL(toggled(bool)),
                   this, SLOT(setSeverityFilter()));
  QObject::connect(ui.checkError, SIGNAL(toggled(bool)),
                   this, SLOT(setSeverityFilter()));
  QObject::connect(ui.checkFatal, SIGNAL(toggled(bool)),
                   this, SLOT(setSeverityFilter()));

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
  QMainWindow::closeEvent(event);
}

void ConsoleWindow::nodeSelectionChanged()
{
  // QModelIndexList selection = ui.nodeList->selectionModel()->selectedIndexes();
  // std::set<std::string> nodes;
  // QStringList node_names;

  // for (size_t i = 0; i < selection.size(); i++) {
  //   std::string name = node_list_model_->nodeName(selection[i]);
  //   nodes.insert(name);
  //   node_names.append(name.c_str());
  // }

  // db_proxy_->setNodeFilter(nodes);

  // for (size_t i = 0; i < node_names.size(); i++) {
  //   node_names[i] = node_names[i].split("/", QString::SkipEmptyParts).last();
  // }
    
  // setWindowTitle(QString("SWRI Console (") + node_names.join(", ") + ")");
}

void ConsoleWindow::setSeverityFilter()
{
  uint8_t mask = 0;

  if (ui.checkDebug->isChecked()) {
    mask |= rosgraph_msgs::Log::DEBUG;
  }
  if (ui.checkInfo->isChecked()) {
    mask |= rosgraph_msgs::Log::INFO;
  }
  if (ui.checkWarn->isChecked()) {
    mask |= rosgraph_msgs::Log::WARN;
  }
  if (ui.checkError->isChecked()) {
    mask |= rosgraph_msgs::Log::ERROR;
  }
  if (ui.checkFatal->isChecked()) {
    mask |= rosgraph_msgs::Log::FATAL;
  }

  QSettings settings;
  settings.setValue(SettingsKeys::SHOW_DEBUG, ui.checkDebug->isChecked());
  settings.setValue(SettingsKeys::SHOW_INFO, ui.checkInfo->isChecked());
  settings.setValue(SettingsKeys::SHOW_WARN, ui.checkWarn->isChecked());
  settings.setValue(SettingsKeys::SHOW_ERROR, ui.checkError->isChecked());
  settings.setValue(SettingsKeys::SHOW_FATAL, ui.checkFatal->isChecked());

  ui.logList->logFilter()->setSeverityMask(mask);
}

void ConsoleWindow::setFont(const QFont &font)
{
  // ui.messageList->setFont(font);
  // ui.nodeList->setFont(font);
}

void ConsoleWindow::setDebugColor()
{
  chooseButtonColor(ui.debugColorWidget);
}

void ConsoleWindow::setInfoColor()
{
  chooseButtonColor(ui.infoColorWidget);
}

void ConsoleWindow::setWarnColor()
{
  chooseButtonColor(ui.warnColorWidget);
}

void ConsoleWindow::setErrorColor()
{
  chooseButtonColor(ui.errorColorWidget);
}

void ConsoleWindow::setFatalColor()
{
  chooseButtonColor(ui.fatalColorWidget);
}

void ConsoleWindow::chooseButtonColor(QPushButton* widget)
{
  QColor old_color = getButtonColor(widget);
  QColor color = QColorDialog::getColor(old_color, this);
  if (color.isValid()) {
    updateButtonColor(widget, color);
  }
}

QColor ConsoleWindow::getButtonColor(const QPushButton* button) const
{
  QString ss = button->styleSheet();
  QRegExp re("background: (#\\w*);");
  QColor old_color;
  if (re.indexIn(ss) >= 0) {
    old_color = QColor(re.cap(1));
  }
  return old_color;
}

void ConsoleWindow::updateButtonColor(QPushButton* widget, const QColor& color)
{
  QString s("background: #"
            + QString(color.red() < 16? "0" : "") + QString::number(color.red(),16)
            + QString(color.green() < 16? "0" : "") + QString::number(color.green(),16)
            + QString(color.blue() < 16? "0" : "") + QString::number(color.blue(),16) + ";");
  widget->setStyleSheet(s);
  widget->update();

  // if (widget == ui.debugColorWidget) {
  //   db_proxy_->setDebugColor(color);
  // }
  // else if (widget == ui.infoColorWidget) {
  //   db_proxy_->setInfoColor(color);
  // }
  // else if (widget == ui.warnColorWidget) {
  //   db_proxy_->setWarnColor(color);
  // }
  // else if (widget == ui.errorColorWidget) {
  //   db_proxy_->setErrorColor(color);
  // }
  // else if (widget == ui.fatalColorWidget) {
  //   db_proxy_->setFatalColor(color);
  // }
  // else {
  //   qWarning("Unexpected widget passed to ConsoleWindow::updateButtonColor.");
  // }
}

void ConsoleWindow::loadColorButtonSetting(const QString& key, QPushButton* button)
{
  QSettings settings;
  QColor defaultColor;
  // The color buttons don't have a default value set in the .ui file, so we need to
  // supply defaults for them here in case the appropriate setting isn't found.
  if (button == ui.debugColorWidget) {
    defaultColor = Qt::gray;
  }
  else if (button == ui.infoColorWidget) {
    defaultColor = Qt::black;
  }
  else if (button == ui.warnColorWidget) {
    defaultColor = QColor(255, 127, 0);
  }
  else if (button == ui.errorColorWidget) {
    defaultColor = Qt::red;
  }
  else if (button == ui.fatalColorWidget) {
    defaultColor = Qt::magenta;
  }
  QColor color = settings.value(key, defaultColor).value<QColor>();
  updateButtonColor(button, color);
}

void ConsoleWindow::loadSettings()
{
  // First, load all the boolean settings...
  loadBooleanSetting(SettingsKeys::DISPLAY_TIMESTAMPS, ui.action_ShowTimestamps);
  loadBooleanSetting(SettingsKeys::ABSOLUTE_TIMESTAMPS, ui.action_AbsoluteTimestamps);
  loadBooleanSetting(SettingsKeys::COLORIZE_LOGS, ui.action_ColorizeLogs);

  // The severity level has to be handled a little differently, since they're all combined
  // into a single integer mask under the hood.  First they have to be loaded from the settings,
  // then set in the UI, then the mask has to actually be applied.
  QSettings settings;
  bool showDebug = settings.value(SettingsKeys::SHOW_DEBUG, true).toBool();
  bool showInfo = settings.value(SettingsKeys::SHOW_INFO, true).toBool();
  bool showWarn = settings.value(SettingsKeys::SHOW_WARN, true).toBool();
  bool showError = settings.value(SettingsKeys::SHOW_ERROR, true).toBool();
  bool showFatal = settings.value(SettingsKeys::SHOW_FATAL, true).toBool();
  ui.checkDebug->setChecked(showDebug);
  ui.checkInfo->setChecked(showInfo);
  ui.checkWarn->setChecked(showWarn);
  ui.checkError->setChecked(showError);
  ui.checkFatal->setChecked(showFatal);
  setSeverityFilter();

  // Load button colors.
  loadColorButtonSetting(SettingsKeys::DEBUG_COLOR, ui.debugColorWidget);
  loadColorButtonSetting(SettingsKeys::INFO_COLOR, ui.infoColorWidget);
  loadColorButtonSetting(SettingsKeys::WARN_COLOR, ui.warnColorWidget);
  loadColorButtonSetting(SettingsKeys::ERROR_COLOR, ui.errorColorWidget);
  loadColorButtonSetting(SettingsKeys::FATAL_COLOR, ui.fatalColorWidget);

  // Finally, load the filter contents.
  QString includeFilter = settings.value(SettingsKeys::INCLUDE_FILTER, "").toString();
  ui.includeText->setText(includeFilter);
  QString excludeFilter = settings.value(SettingsKeys::EXCLUDE_FILTER, "").toString();
  ui.excludeText->setText(excludeFilter);
  // This triggers the processing, which currently saves the values,
  // so we have to do it after we load the settings.
  loadBooleanSetting(SettingsKeys::USE_REGEXPS, ui.action_RegularExpressions);
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

  QSettings settings;
  settings.setValue(SettingsKeys::USE_REGEXPS, use_re);

  QRegExp re = regExpFromText(ui.includeText->text(), use_re);
  if (re.isValid()) {
    ui.includeLabel->setStyleSheet("QLabel { }");
    ui.logList->logFilter()->setIncludeRegExp(re);
    settings.setValue(SettingsKeys::INCLUDE_FILTER, ui.includeText->text());
  } else {
    ui.includeLabel->setStyleSheet("QLabel { background-color : red; color : white; }");
  }

  re = regExpFromText(ui.excludeText->text(), use_re);
  if (re.isValid()) {
    ui.excludeLabel->setStyleSheet("QLabel { }");
    ui.logList->logFilter()->setExcludeRegExp(re);
    settings.setValue(SettingsKeys::EXCLUDE_FILTER, ui.excludeText->text());
  } else {
    ui.excludeLabel->setStyleSheet("QLabel { background-color : red; color : white; }");
  }  
}
}  // namespace swri_console

