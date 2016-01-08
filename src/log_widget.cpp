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

#include <QApplication>
#include <QClipboard>
#include <QDebug>
#include <QEvent>
#include <QPainter>
#include <QScrollBar>
#include <QStyle>
#include <QStyleOptionViewItemV4>
#include <QTime>
#include <QTimer>
#include <QMouseEvent>
#include <QHelpEvent>
#include <QKeyEvent>
#include <QToolTip>

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
  QObject::connect(verticalScrollBar(), SIGNAL(valueChanged(int)),
                   this, SLOT(handleScrollChanged()));
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

  startTimer(50);
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

  if (auto_scroll_to_bottom_) {
    verticalScrollBar()->setValue(verticalScrollBar()->maximum());
  }
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
  viewport()->update();
}

void LogWidget::selectAll()
{
  qWarning("select all");
  if (!blocks_.empty()) {
    selection_start_ = RowIndex(0,0);
    selection_stop_ = RowIndex(blocks_.size()-1, blocks_.back().rows.size()-1);
    viewport()->update();
  }
}

void LogWidget::copyLogsToClipboard()
{
  if (!selection_start_.isValid()) {
    return;
  }

  RowIndex begin_row = std::min(selection_start_, selection_stop_);
  RowIndex end_row = std::max(selection_start_, selection_stop_);

  QStringList buffer;
  for (RowIndex row = begin_row; row <= end_row; adjustRow(row, 1)) {
    const SessionData &session_data = blocks_[row.session_idx];
    const Session &session = db_->session(session_data.session_id);
    RowMap line_map = session_data.rows[row.row_idx];
    Log log = session.log(line_map.log_index);
    
    buffer.append(logText(log, line_map.line_index));    
  }

  QApplication::clipboard()->setText(buffer.join(tr("\n")));  
}

void LogWidget::copyExtendedLogsToClipboard()
{
  if (!selection_start_.isValid()) {
    return;
  }

  RowIndex begin_row = std::min(selection_start_, selection_stop_);
  RowIndex end_row = std::max(selection_start_, selection_stop_);

  int last_session_idx = -1;
  int last_log_idx = -1;
  
  QStringList buffer;
  for (RowIndex row = begin_row; row <= end_row; adjustRow(row, 1)) {
    const SessionData &session_data = blocks_[row.session_idx];
    const Session &session = db_->session(session_data.session_id);
    RowMap line_map = session_data.rows[row.row_idx];
    
    if (last_session_idx == row.session_idx && last_log_idx == line_map.log_index) {
      continue;
    }
    last_session_idx = row.session_idx;
    last_log_idx = line_map.log_index;
    
    Log log = session.log(line_map.log_index);
    buffer.append(extendedLogRole(log));
  }

  QString separator = tr("\n\n========================================\n\n");
  QApplication::clipboard()->setText(buffer.join(separator));  
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

  if (blocks_.size()) {
    current_row_ = RowIndex(blocks_.size()-1, 0);
  } else {
    current_row_ = RowIndex();
  }
  clearSelection();
  
  scheduleIdleProcessing();
  viewport()->update();
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
  // When processing old logs, we iterate backwards through the blocks
  // and their logs to get better behavior when follow latest messages
  // is selected (ie, most recent messages are added first).
  size_t count = 0;
  QTime process_time;
  process_time.start();
  for (size_t i = 0; i < blocks_.size(); i++) {
    SessionData &block = blocks_[blocks_.size()-i-1];

    if (block.earliest_log_index == 0) {
      // Nothing to do for this block.
      continue;
    }

    std::vector<RowMap> early_rows;
    
    const Session &session = db_->session(block.session_id);
    if (!session.isValid()) {
      qWarning("Invalid session in %s?", __PRETTY_FUNCTION__);
      continue;
    }

    while (block.earliest_log_index != 0 && process_time.elapsed() < 20) {
      for (size_t i = 0;
           block.earliest_log_index != 0 && i < 100;
           block.earliest_log_index--, i++)
      {
        count++;
        const Log log = session.log(block.earliest_log_index-1);
        if (!filter_->accept(log)) {
          continue;
        }

        size_t line_count = log.lineCount();
        for (int r = 0; r < line_count; r++) {
          // Note that we have to add the lines backwards to maintain the proper order.
          early_rows.push_back(RowMap(block.earliest_log_index-1,
                                      line_count-1-r));
        }
      }
    }

    // qWarning("intermediate processed %d msgs, kept %zu in %d ms",
    //          count, early_rows.size(), process_time.elapsed());

    if (early_rows.size()) {
      size_t start_row = 0;
      for (size_t r = 0; r < i; r++) {
        start_row += blocks_[r].rows.size();
      }

      block.rows.insert(block.rows.begin() + 1,
                        early_rows.rbegin(),
                        early_rows.rend());
      block.alternate_base += early_rows.size();
      updateRowCount(row_count_ + early_rows.size());
      Q_EMIT messagesAdded();
      viewport()->update();
    }
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

      size_t line_count = log.lineCount();
      for (int r = 0; r < line_count; r++) {
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
  updateGeometry();
}

bool LogWidget::event(QEvent *event)
{
  if (event->type() == QEvent::ToolTip) {
    toolTipEvent(static_cast<QHelpEvent*>(event));
    return true;
  }
  
  return QAbstractScrollArea::event(event);
}

void LogWidget::toolTipEvent(QHelpEvent *event)
{
  RowIndex row = indexAt(event->pos());
  if (row.isValid()) {    
    QToolTip::showText(event->globalPos(), toolTip(row), this);
  }
}

QString LogWidget::logText(const Log &log, int line_index) const
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

  return QString(header) + log.textLine(line_index);
}

QString LogWidget::toolTip(const RowIndex &row) const
{
  const SessionData &session_data = blocks_[row.session_idx];
  const Session &session = db_->session(session_data.session_id);
  RowMap line_map = session_data.rows[row.row_idx];
  Log log = session.log(line_map.log_index);
  QString text("<p style='white-space:pre'>");
  text += extendedLogRole(log);
  text += QString("</p>");
  return text;
}

QString LogWidget::extendedLogRole(const Log &log) const
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

const QColor& LogWidget::logColor(const Log &log) const
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

void LogWidget::focusInEvent(QFocusEvent *event)
{
  QAbstractScrollArea::focusInEvent(event);
  viewport()->update();
}

void LogWidget::focusOutEvent(QFocusEvent *event)
{
  QAbstractScrollArea::focusOutEvent(event);
  viewport()->update();
}

void LogWidget::resizeEvent(QResizeEvent *event)
{
  QAbstractScrollArea::resizeEvent(event);
  updateGeometry();
}

void LogWidget::updateGeometry()
{
  QStyleOptionViewItemV4 option = viewOptions();
  row_height_ = style()->sizeFromContents(QStyle::CT_ItemViewItem, &option, QSize(), this).height();

  if (row_height_ == 0) {
    display_row_count_ = 0;
  } else {    
    display_row_count_ = std::floor(static_cast<double>(viewport()->size().height()) / row_height_);
  }

  bool blocked = verticalScrollBar()->blockSignals(true);
  verticalScrollBar()->setRange(0, std::max(0uL, row_count_ - display_row_count_));
  verticalScrollBar()->setPageStep(display_row_count_);
  if (auto_scroll_to_bottom_) {
    verticalScrollBar()->setValue(verticalScrollBar()->maximum());
  }
  verticalScrollBar()->blockSignals(blocked);
  updateLayout();
}

void LogWidget::updateLayout()
{
  if (blocks_.empty()) {
    top_offset_px_ = 0;
    top_row_.session_idx = -1;
    top_row_.row_idx = 0;
    return;
  }
   
  QScrollBar *vbar = verticalScrollBar();
  if (vbar->maximum() - vbar->minimum() == 0) {
    top_offset_px_ = 0;
    top_row_.session_idx = 0;
    top_row_.row_idx = 0;    
  } else if (vbar->value() == vbar->maximum()) {
    top_offset_px_ = 0;
    top_row_.session_idx = blocks_.size()-1;
    top_row_.row_idx = blocks_.back().rows.size()-1;
    int count = adjustRow(top_row_, -(display_row_count_));
    if (count == display_row_count_) {
      top_offset_px_ = row_height_*(display_row_count_+1) - viewport()->size().height();
    }
  } else {
    int session_idx = -1;
    size_t row_idx = vbar->value();
    for (size_t i = 0; i < blocks_.size(); i++) {
      if (row_idx < blocks_[i].rows.size()) {
        session_idx = i;
        break;
      }
      row_idx -= blocks_[i].rows.size();
    }

    if (session_idx < 0) {
      qWarning("Error in update layout");
      top_offset_px_ = 0;
      top_row_.session_idx = 0;
      top_row_.row_idx = 0;
    } else {
      top_offset_px_ = 0;
      top_row_.session_idx = session_idx;
      top_row_.row_idx = row_idx;
    }    
  }

  // qWarning("align pixel: %d align_session: %d align_row: %zu",
  //          top_offset_px_, top_row_.session_idx, top_row_.row_idx);
}

void LogWidget::paintEvent(QPaintEvent *)
{
  QPainter painter(viewport());

  if (blocks_.empty()) {
    return;
  }

  QStyleOptionViewItemV4 option_proto = viewOptions();

  int width = viewport()->rect().width();
  int max_y = viewport()->rect().height();

  const bool focus = (hasFocus() || viewport()->hasFocus()) && current_row_.isValid();

  int y = -top_offset_px_;
  RowIndex row = top_row_;

  while (y < max_y) {
    auto const &block = blocks_[row.session_idx];
    QStyleOptionViewItemV4 option = option_proto;
    option.rect = QRect(0, y, width, row_height_);

    // option.state |= QStyle::State_Enabled;

    if (focus && current_row_ == row) {
      option.state |= QStyle::State_HasFocus;
    }

    if (isSelected(row)) {
      option.state |= QStyle::State_Selected;
    }
    
    fillOption(option, block, row.row_idx);
    
    style()->drawControl(QStyle::CE_ItemViewItem, &option, &painter, this);
    if (!adjustRow(row, 1)) {
      break;
    }
    y += row_height_;
  }
}

QStyleOptionViewItemV4 LogWidget::viewOptions()
{
  QStyleOptionViewItemV4 option;
  option.initFrom(this);
  option.font = font();
  option.state &= ~QStyle::State_MouseOver;
  option.state &= ~QStyle::State_HasFocus;
  if (!hasFocus())
    option.state &= ~QStyle::State_Active;

  int pm = style()->pixelMetric(QStyle::PM_ListViewIconSize, 0, this);
  option.decorationSize = QSize(pm, pm);

  option.decorationPosition = QStyleOptionViewItem::Left;
  option.decorationAlignment = Qt::AlignCenter;

  option.displayAlignment = Qt::AlignLeft | Qt::AlignVCenter;
  option.textElideMode = Qt::ElideNone;
  // This has to be true or the selection/focus rect will only cover
  // the text.
  option.showDecorationSelected = true;

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
    RowMap line_map = session_data.rows[row_idx];
    Log log = session.log(line_map.log_index);

    option.features |= QStyleOptionViewItemV2::HasDisplay;
    option.text = logText(log, line_map.line_index);
    option.palette.setBrush(QPalette::Text, logColor(log));
    option.displayAlignment = Qt::AlignLeft | Qt::AlignTop;

    if ((row_idx - session_data.alternate_base) % 2) {
      option.backgroundBrush = QColor(240, 240, 240);
    }
  }
}

int LogWidget::adjustRow(RowIndex &row, int offset) const
{
  if (row.session_idx >= blocks_.size()) {
    return 0; 
  } else if (row.row_idx >= blocks_[row.session_idx].rows.size()) {
    return 0;
  }
  
  int count = 0;
  if (offset < 0) {
    while (offset < 0) {
      if (row.row_idx == 0) {
        if (row.session_idx == 0) {
          break;
        } else {
          row.session_idx--;
          row.row_idx = blocks_[row.session_idx].rows.size()-1;
        }
      } else {
        row.row_idx--;
      }
      count++;
      offset++;
    }
  } else {
    while (offset > 0) {
      if (row.row_idx+1 == blocks_[row.session_idx].rows.size()) {
        if (row.session_idx+1 == blocks_.size()) {
          break;
        } else {
          row.session_idx++;
          row.row_idx = 0;
            }
      } else {
        row.row_idx++;
      }
      count++;
      offset--;
    }
  }
  return count;      
}

LogWidget::RowIndex LogWidget::indexAt(const QPoint &pos) const
{
  if (!top_row_.isValid()) {
    return RowIndex();
  }

  double y = pos.y() + top_offset_px_;
  int display_line = y / row_height_;

  RowIndex row = top_row_;
  int adjusted = adjustRow(row, display_line);
  if (adjusted != display_line) {
    return RowIndex();
  } else {
    return row;
  }
}

void LogWidget::mousePressEvent(QMouseEvent *event)
{
  if (event->button() == Qt::LeftButton) {  
    RowIndex row = indexAt(event->pos());
    if (!row.isValid()) {
      return;
    }

    if (row != current_row_) { 
      current_row_ = row;

      if (event->modifiers() == Qt::ShiftModifier) {
        if (!selection_start_.isValid()) {
          setSelection(current_row_);
        } else {
          selection_stop_ = current_row_;
        }      
      } else {
        setSelection(current_row_);
      }
      scrollToIndex(current_row_);
      viewport()->update();    
    }
    event->accept();
    return;
  }
  
  QAbstractScrollArea::mousePressEvent(event);
}

void LogWidget::keyPressEvent(QKeyEvent *event)
{
  if (blocks_.empty()) {
    return;
  }

  RowIndex start_row = current_row_;
  if (!start_row.isValid()) {
    start_row = top_row_;
  }    

  RowIndex end_row = start_row;
  if (event->key() == Qt::Key_Down) {
    adjustRow(end_row, 1);
  } else if (event->key() == Qt::Key_Up) {
    adjustRow(end_row, -1);
  } else if (event->key() == Qt::Key_PageDown) {
    adjustRow(end_row, display_row_count_-1);
  } else if (event->key() == Qt::Key_PageUp) {
    adjustRow(end_row, -(display_row_count_-1));
  } else if (event->key() == Qt::Key_Home) {
    // todo: home should go to start of current session, or start of
    // previous session if current row is top of current session.
    end_row = RowIndex(0,0);
  } else if (event->key() == Qt::Key_End) {
    // todo: end should go to end of current session, or end of next
    // session if current row is end of current session.
    end_row = RowIndex(blocks_.size()-1,
                       blocks_.back().rows.size()-1);
  }

  if (start_row != end_row) {
    current_row_ = end_row;

    if (event->modifiers() != Qt::ShiftModifier) {
      setSelection(current_row_);
    } else {
      if (!selection_start_.isValid()) {
        selection_start_ = current_row_;
      }
      selection_stop_ = current_row_;
    }
    
    scrollToIndex(current_row_);
    event->accept();
    viewport()->update();
    return;
  }

  if (event->key() == Qt::Key_A && (event->modifiers() == Qt::ControlModifier)) {
    selectAll();
    event->accept();
    viewport()->update();
    return;
  }

  QAbstractScrollArea::keyPressEvent(event);
}

void LogWidget::scrollToIndex(const RowIndex &row)
{
  // If the row is currently visible, do nothing. If the row is above
  // the top row, scroll so that it becomes the top row. If the row is
  // below the bottom row, scroll so that it becomes the bottom row.

  // assume top row is valid.

  RowIndex bottom_row = top_row_;
  adjustRow(bottom_row, display_row_count_-1);

  if (row < top_row_) {
    size_t idx = displayIndexForRow(row);
    verticalScrollBar()->setValue(idx);
  } else if (bottom_row < row) {
    size_t idx = displayIndexForRow(row);
    verticalScrollBar()->setValue(idx-(display_row_count_-1));    
  } else {
    // Nothing to do.
  }  
}

size_t LogWidget::displayIndexForRow(const RowIndex &row) const
{
  size_t index = row.row_idx;
  for (size_t i = 0; i < row.session_idx; i++) {
    index += blocks_[i].rows.size();
  }
  return index;
}

void LogWidget::handleScrollChanged()
{
  if (auto_scroll_to_bottom_) {
    if (verticalScrollBar()->value() < verticalScrollBar()->maximum()) {
      setAutoScrollToBottom(false);
    }
  } else {
    if (verticalScrollBar()->value() == verticalScrollBar()->maximum()) {
      setAutoScrollToBottom(true);
    }
  }
  updateLayout();
}

void LogWidget::clearSelection()
{
  selection_start_ = RowIndex();
  selection_stop_ = RowIndex();
}

void LogWidget::setSelection(const RowIndex &index)
{
  selection_start_ = index;
  selection_stop_ = index;
}

bool LogWidget::isSelected(const RowIndex &index) const
{
  if (!selection_start_.isValid()) { return false; }

  if (selection_start_ <= selection_stop_) {
    return (selection_start_ <= index && index <= selection_stop_);
  } else {
    return (selection_stop_ <= index && index <= selection_start_);
  }
}
}  // namespace swri_console
