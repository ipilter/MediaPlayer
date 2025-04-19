#include "Slider.h"

#include <QMouseEvent>
#include <QMediaPlayer>
#include <QPainter>
#include <QStyleOption>

Slider::Slider(Qt::Orientation orientation, QWidget* parent)
  : QSlider(orientation, parent)
{
  setValue(0);
}

void Slider::mousePressEvent(QMouseEvent* event)
{
  if (event->button() == Qt::LeftButton)
  {
    const int value = QStyle::sliderValueFromPosition(minimum(), maximum(), event->pos().x(), width());
    setValue(value);
    emit sliderMoved(value);
  }
  QSlider::mousePressEvent(event);
}

void Slider::setSequences(const SequenceMap& sequences)
{
  mSequences = sequences;
  update();
}
const QColor& Slider::sequenceColor(const SequenceEntry& sequence) const
{
  static const std::map<QString, const QColor> colorMap = {
    {"invalid", 0xFF0000},
    {"ready", 0x2B65BC},
    {"processing", 0xB5AA48},
    {"succeeded", 0x72D64A},
    {"failed", 0xB52B2B}
  };

  QString colorName = "invalid";
  switch (sequence.second)
  {
  case SequenceState::Ready:
    colorName = "ready";
    break;
  case SequenceState::Processing:
    colorName = "processing";
    break;
  case SequenceState::Succeeded:
    colorName = "succeeded";
    break;
  case SequenceState::Failed:
    colorName = "failed";
    break;
  }

  return colorMap.at(colorName);
}

void Slider::paintEvent(QPaintEvent* event)
{
  QSlider::paintEvent(event);
  QPainter painter(this);

  const int width = this->width();
  const int height = this->height();

  for (const auto& sequence : mSequences)
  {
    const int startX = QStyle::sliderPositionFromValue(minimum(), maximum(), static_cast<int>(sequence.first.first.ms()), width);
    const int endX = QStyle::sliderPositionFromValue(minimum(), maximum(), static_cast<int>(sequence.first.second.ms()), width);
    const QRect rect(startX, 5, endX - startX, 7);
    
    painter.fillRect(rect, sequenceColor(sequence));
  }
}
