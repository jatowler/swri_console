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
#include <swri_console/log_widget.h>

#include <QDebug>
#include <QEvent>
#include <QPainter>
#include <QScrollBar>
#include <QStyle>
#include <QStyleOptionViewItemV4>
#include <QTimer>

#include <swri_console/log.h>
#include <swri_console/log_database.h>
#include <swri_console/log_filter.h>

namespace swri_console
{
LogWidget::LogWidget(QWidget *parent)
  :
  QAbstractScrollArea(parent),
  db_(NULL),
  filter_(new LogFilter(this)),
  auto_scroll_to_bottom_(true),
  stamp_format_(STAMP_FORMAT_RELATIVE),
  debug_color_(Qt::gray),
  info_color_(Qt::black),
  warn_color_(QColor(255,127,0)),
  error_color_(Qt::red),
  fatal_color_(Qt::magenta),
  row_count_(0)
{
  setVerticalScrollBarPolicy(Qt::ScrollBarAlwaysOn);
  
  QObject::connect(filter_, SIGNAL(filterModified()),
                   this, SLOT(filterModified()));
}

LogWidget::~LogWidget()
{
}

void LogWidget::setDatabase(LogDatabase *db)
{
  if (db_) {
    // We can implement this, just don't have a use case currently.
    qWarning("LogWidget: Cannot change the database.");
    return;
  }

  db_ = db;

  QObject::connect(db_, SIGNAL(sessionMinTimeChanged(int)),
                   this, SLOT(allDataChanged()));
  QObject::connect(db_, SIGNAL(databaseCleared()),
                   this, SLOT(handleDatabaseCleared()));

  reset();  
}

DatabaseView LogWidget::selectedLogContents() const
{
  return DatabaseView();
}

DatabaseView LogWidget::displayedLogContents() const
{
    return DatabaseView();
}

DatabaseView LogWidget::sessionsLogContents() const
{
    return DatabaseView();
}

DatabaseView LogWidget::allLogContents() const
{
    return DatabaseView();
}

void LogWidget::setSessionFilter(const std::vector<int> &sids)
{
  sids_ = sids;
  // Note: We could do a partial reset here...
  reset();
}

void LogWidget::filterModified()
{
  reset();
}

void LogWidget::setAutoScrollToBottom(bool auto_scroll)
{
  if (auto_scroll_to_bottom_ == auto_scroll) {
    return;
  }

  auto_scroll_to_bottom_ = auto_scroll;
  Q_EMIT autoScrollToBottomChanged(auto_scroll_to_bottom_);

  // if (auto_scroll_to_bottom_) {
  //   list_view_->scrollToBottom();
  // }
}

void LogWidget::setStampFormat(const StampFormat &format)
{
  if (stamp_format_ == format) { return; }
  stamp_format_ = format;
  allDataChanged();
}

void LogWidget::setDebugColor(const QColor &color)
{
  debug_color_ = color;
  allDataChanged();
}

void LogWidget::setInfoColor(const QColor &color)
{
  info_color_ = color;
  allDataChanged();
}

void LogWidget::setWarnColor(const QColor &color)
{
  warn_color_ = color;
  allDataChanged();
}

void LogWidget::setErrorColor(const QColor &color)
{
  error_color_ = color;
  allDataChanged();
}

void LogWidget::setFatalColor(const QColor &color)
{
  fatal_color_ = color;
  allDataChanged();
}

void LogWidget::allDataChanged()
{
  // repaint
}

void LogWidget::selectAll()
{
}

void LogWidget::copyLogsToClipboard()
{
}

void LogWidget::copyExtendedLogsToClipboard()
{
}

void LogWidget::reset()
{
  blocks_.clear();
  for (size_t i = 0; i < sids_.size(); i++) {
    const Session &session = db_->session(sids_[i]);
    if (!session.isValid()) {
      qWarning("LogWidget::reset got invalid session %d", sids_[i]);
      continue;
    }

    blocks_.emplace_back();
    SessionData &block = blocks_.back();    
    block.session_id = sids_[i];
    // Insert one item that be a placeholder for the session header.
    block.rows.push_back(RowMap());
    block.latest_log_index = session.logCount();
    block.earliest_log_index = session.logCount();
    block.alternate_base = 0;
  }
  updateRowCount(blocks_.size());
  scheduleIdleProcessing();
}

void LogWidget::handleDatabaseCleared()
{
  sids_.clear();
  blocks_.clear();
}

void LogWidget::scheduleIdleProcessing()
{
  // If we have older logs that still need to be processed, schedule a
  // callback at the next idle time.
  for (size_t i = 0; i < blocks_.size(); i++) {
    if (blocks_[i].earliest_log_index > 0) {
      QTimer::singleShot(0, this, SLOT(processOldMessages()));
      return;
    }
  }
}

void LogWidget::processOldMessages()
{
  // We process old messages in two steps.  First, we process the
  // remaining messages in chunks and store them in a early_rows
  // buffer if they pass all the filters.  When the early mapping
  // buffer is large enough (or we have processed everything for that
  // session), then we merge the early_rows buffer in the main
  // buffer.  This approach allows us to process very large logs
  // without causing major lag for the user.
  //
  // Unlike processNewMessages, we only process old messages for one
  // session at time.  This is because the number of unprocess old
  // messages is bounded, so we will eventually get through all of
  // them.  We could process all of them, but then each processing
  // time chunk would be grow with how many sessions are selected, and
  // we risk making the GUI unresponsive is a lot of sessions are active.
  //
  // Also, we iterate through the blocks backwards for this to get
  // better behavior when follow latest messages is selected.
  for (size_t i = 0; i < blocks_.size(); i++) {
    SessionData &block = blocks_[blocks_.size()-i-1];
    
    if (block.earliest_log_index == 0) {
      // Nothing to do for this block.
      continue;
    }
    
    const Session &session = db_->session(block.session_id);
    if (!session.isValid()) {
      qWarning("Invalid session in %s?", __PRETTY_FUNCTION__);
      continue;
    }

    for (size_t i = 0;
         block.earliest_log_index != 0 && i < 100;
         block.earliest_log_index--, i++)
    {
      const Log log = session.log(block.earliest_log_index-1);
      if (!filter_->accept(log)) {
         continue;
      }
      
      QStringList text_lines = log.textLines();
      for (int r = 0; r < text_lines.size(); r++) {
        // Note that we have to add the lines backwards to maintain the proper order.
        block.early_rows.push_front(RowMap(block.earliest_log_index-1,
                                            text_lines.size()-1-r));
      }
    }

    if ((block.earliest_log_index == 0 && block.early_rows.size()) ||
        block.early_rows.size() > 200) {
      size_t start_row = 0;
      for (size_t r = 0; r < i; r++) {
        start_row += blocks_[r].rows.size();
      }

      block.rows.insert(block.rows.begin() + 1, 
                        block.early_rows.begin(),
                        block.early_rows.end());
      block.alternate_base += block.early_rows.size();
      updateRowCount(row_count_ + block.early_rows.size());
      block.early_rows.clear();
      Q_EMIT messagesAdded();
      viewport()->update();      
    }

    // Don't process any more blocks.
    break;
  }

  scheduleIdleProcessing();
}

void LogWidget::processNewMessages()
{  
  bool messages_added = false;
  for (size_t i = 0; i < blocks_.size(); i++) {
    SessionData &block = blocks_[i];
    const Session &session = db_->session(block.session_id);
    if (!session.isValid()) {
      qWarning("Invalid session in %s?", __PRETTY_FUNCTION__);
      continue;
    }

    std::deque<RowMap> new_items;
    for(; block.latest_log_index < session.logCount(); block.latest_log_index++) {
      const Log log = session.log(block.latest_log_index);

      if (!filter_->accept(log)) {
        continue;
      }

      QStringList text_lines = log.textLines();
      for (int r = 0; r < text_lines.size(); r++) {
        new_items.push_back(RowMap(block.latest_log_index, r));
      }
    }

    if (!new_items.empty()) {
      size_t start_row = 0;
      for (size_t r = 0; r <= i; r++) {
        start_row += blocks_[r].rows.size();
      }
      
      block.rows.insert(block.rows.end(),
                        new_items.begin(),
                        new_items.end());
      updateRowCount(row_count_ + new_items.size());
      messages_added = true;
    }
  }

  if (messages_added) {
    Q_EMIT messagesAdded();
    viewport()->update();
  }
}

void LogWidget::timerEvent(QTimerEvent*)
{
  processNewMessages();
}

void LogWidget::updateRowCount(size_t row_count)
{
  row_count_ = row_count;
  verticalScrollBar()->setRange(0, row_count);  
}

bool LogWidget::event(QEvent *event)
{
  return QAbstractScrollArea::event(event);
}

void LogWidget::paintEvent(QPaintEvent *)
{
  QPainter painter(viewport());
  updateRowHeight();
    
  if (blocks_.empty()) {
    return;
  }

  QStyleOptionViewItemV4 option_proto = viewOptions();

  int width = viewport()->rect().width();
  int max_y = viewport()->rect().height();
  int y = 0;

  for (size_t i = 0; i < blocks_.size(); i++) {
    auto const &block = blocks_[i];

    if (y > max_y) { break; }
    for (size_t j = 0; j < block.rows.size(); j++) {
      QStyleOptionViewItemV4 option = option_proto;
      option.rect = QRect(0, y, width, row_height_);      
      fillOption(option, block, j);
            
      style()->drawControl(QStyle::CE_ItemViewItem, &option, &painter, this);  
      y += row_height_;
      if (y > max_y) { break; }
    }
  }      
}

QVariant LogWidget::data(size_t session_idx, size_t row_idx, int role) const
{
  switch (role)
  {
    // Currently we're only returning data for these roles, so return immediately
    // if we're being queried for anything else.
    case Qt::DisplayRole:
    case Qt::ToolTipRole:
    case Qt::ForegroundRole:
    case Qt::BackgroundRole:
    // case ExtendedLogRole:
    case Qt::TextAlignmentRole:
      break;
    default:
      return QVariant();
  }

  if (row_idx == 0) {
    return separatorData(session_idx, role);
  }
  
  int sid = blocks_[session_idx].session_id;
  RowMap line_map = blocks_[session_idx].rows[row_idx];
  size_t lid = line_map.log_index;
  Log log = db_->session(sid).log(lid);
  
  if (!log.isValid()) {
    return QVariant();
  }

  if (role == Qt::DisplayRole) {
    return displayRole(log, line_map.line_index);
  } else if (role == Qt::ToolTipRole) {
    return toolTipRole(log, line_map.line_index);
  } else if (role == Qt::ForegroundRole) {
    return foregroundRole(log, line_map.line_index);
  } else if (role == Qt::BackgroundRole) {
    return backgroundRole(session_idx, row_idx);
  // } else if (role == ExtendedLogRole) {
  //   return extendedLogRole(log, line_map.line_index);
  } else {
    return QVariant();
  }

  return QVariant();
}

QVariant LogWidget::separatorData(int session_idx, int role) const
{
  const SessionData &data = blocks_[session_idx];
  const Session &session = db_->session(data.session_id);
  
  if (role == Qt::DisplayRole) {
    return session.name();
  } else if (role == Qt::ToolTipRole) {
    return session.name();
  } else if (role == Qt::ForegroundRole) {
    return Qt::white;
  } else if (role == Qt::BackgroundRole) {
    return QColor(110, 110, 110);
    
  // } else if (role == ExtendedLogRole) {
  //   return QString("\n ------ %1 ----- \n").arg(session.name());
  } else if (role == Qt::TextAlignmentRole) {
    return Qt::AlignHCenter;
  } else {
    return QVariant();
  }  
}

QVariant LogWidget::displayRole(const Log &log, int line_index) const
{  
  char severity = '?';
  if (log.severity() == rosgraph_msgs::Log::DEBUG) {
    severity = 'D';
  } else if (log.severity() == rosgraph_msgs::Log::INFO) {
    severity = 'I';
  } else if (log.severity() == rosgraph_msgs::Log::WARN) {
    severity = 'W';
  } else if (log.severity() == rosgraph_msgs::Log::ERROR) {
    severity = 'E';
  } else if (log.severity() == rosgraph_msgs::Log::FATAL) {
    severity = 'F';
  }

  QString header;
  if (stamp_format_ == STAMP_FORMAT_NONE) {
    header = QString("[%1] ").arg(severity);
  } else if (stamp_format_ == STAMP_FORMAT_RELATIVE) {
    ros::Time t = log.relativeTime();
    int32_t secs = t.sec;
    int hours = secs / 60 / 60;
    int minutes = (secs / 60) % 60;
    int seconds = (secs % 60);
    int milliseconds = t.nsec / 1000000;

    char stamp[128];
    snprintf(stamp, sizeof(stamp),
             "%d:%02d:%02d:%03d",
             hours, minutes, seconds, milliseconds);
    
    header = QString("[%1 %2] ").arg(severity).arg(stamp);
  } else {
    ros::Time t = log.absoluteTime();
    char stamp[128];
    snprintf(stamp, sizeof(stamp), 
             "%u.%09u",
             t.sec,
             t.nsec);
    header = QString("[%1 %2] ").arg(severity).arg(stamp);
  }

  // For multiline messages, we only want to display the header for
  // the first line.  For the subsequent lines, we generate a header
  // and then fill it with blank lines so that the messages are
  // aligned properly (assuming monospaced font).  
  if (line_index != 0) {
    header.fill(' ');
  }
    
  return QString(header) + log.textLines()[line_index];
}

QVariant LogWidget::toolTipRole(const Log &log, int line_index) const
{           
  QString text("<p style='white-space:pre'>");
  text += extendedLogRole(log, line_index).toString();
  text += QString("</p>");
  return text;
}

QVariant LogWidget::extendedLogRole(const Log &log, int line_index) const
{
  char stamp[128];
  snprintf(stamp, sizeof(stamp),
           "%d.%09d",
           log.absoluteTime().sec,
           log.absoluteTime().nsec);

  QString text;
  text += QString("Timestamp: %1\n").arg(stamp);
  text += QString("Node: %1\n").arg(log.nodeName());
  text += QString("Function: %1\n").arg(log.functionName());
  text += QString("File: %1\n").arg(log.fileName());
  text += QString("Line: %1\n").arg(log.lineNumber());
  text += QString("\n");
  text += log.textLines().join("\n");
  return text;
}

QVariant LogWidget::foregroundRole(const Log &log, int) const
{
  switch (log.severity()) {
  case rosgraph_msgs::Log::DEBUG:
    return debug_color_;
  case rosgraph_msgs::Log::INFO:
    return info_color_;
  case rosgraph_msgs::Log::WARN:
    return warn_color_;
  case rosgraph_msgs::Log::ERROR:
    return error_color_;
  case rosgraph_msgs::Log::FATAL:
    return fatal_color_;
  default:
    return info_color_;
  }
}

QVariant LogWidget::backgroundRole(int session_idx, int row_idx) const
{
}

void LogWidget::updateRowHeight()
{
  QStyleOptionViewItemV4 option = viewOptions();
  int row_height = style()->sizeFromContents(QStyle::CT_ItemViewItem, &option, QSize(), this).height();

  if (row_height == row_height_) {
    return;
  }

  row_height_ = row_height;
}

QStyleOptionViewItemV4 LogWidget::viewOptions()
{
  QStyleOptionViewItemV4 option;
  option.initFrom(this);
  option.state &= ~QStyle::State_MouseOver;
  option.font = font();
  option.state &= ~QStyle::State_HasFocus;

  {
    int pm = style()->pixelMetric(QStyle::PM_ListViewIconSize, 0, this);
    option.decorationSize = QSize(pm, pm);
  }
  
  option.decorationPosition = QStyleOptionViewItem::Left;
  option.decorationAlignment = Qt::AlignCenter;
  
  option.displayAlignment = Qt::AlignLeft | Qt::AlignVCenter;
  option.textElideMode = Qt::ElideRight;
  option.showDecorationSelected = false;
  
  option.locale = locale();
  option.locale.setNumberOptions(QLocale::OmitGroupSeparator);
  option.widget = this;
  return option;
}

void LogWidget::fillOption(QStyleOptionViewItemV4 &option,
                           const SessionData &session_data, int row_idx)
{
  if (row_idx == 0) {
    const Session &session = db_->session(session_data.session_id);
    option.features |= QStyleOptionViewItemV2::HasDisplay;
    option.text = session.name();
    option.palette.setBrush(QPalette::Text, Qt::white);
    option.backgroundBrush = QColor(110, 110, 110);
    option.displayAlignment = Qt::AlignHCenter | Qt::AlignTop;
  } else {
    const Session &session = db_->session(session_data.session_id);
    
    option.displayAlignment = Qt::AlignLeft | Qt::AlignTop;
    option.features |= QStyleOptionViewItemV2::HasDisplay;
        
    if ((row_idx - session_data.alternate_base) % 2) {
      option.backgroundBrush = QColor(240, 240, 240);
    }
  }
}
}  // namespace swri_console
