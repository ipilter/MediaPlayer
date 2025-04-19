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

void Slider::setSequences(const Sequences& sequences)
{
  mSequences.clear();
  mSequences.insert(mSequences.end(), sequences.begin(), sequences.end());
  update();
}

void Slider::paintEvent(QPaintEvent* event)
{
  QSlider::paintEvent(event);
  QPainter painter(this);

  const int width = this->width();
  const int height = this->height();
  const QColor rectColor = palette().color(QPalette::Light);

  for (const auto& sequence : mSequences)
  {
    const int startX = QStyle::sliderPositionFromValue(minimum(), maximum(), static_cast<int>(sequence.first.ms()), width);
    const int endX = QStyle::sliderPositionFromValue(minimum(), maximum(), static_cast<int>(sequence.second.ms()), width);
    const QRect rect(startX, 0, endX - startX, 10);

    painter.fillRect(rect, rectColor);
  }
}