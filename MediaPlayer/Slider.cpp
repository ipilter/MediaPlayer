#include "Slider.h"

#include <QMouseEvent>
#include <QMediaPlayer>
#include <QPainter>
#include <QStyleOption>
#include <QProxyStyle>
#include <QStyleHintReturn>

class SliderStyle : public QProxyStyle
{
public:
  SliderStyle() : QProxyStyle() {}

  int styleHint(StyleHint hint, const QStyleOption* option, const QWidget* widget, QStyleHintReturn* returnData) const override
  {
    if (hint == QStyle::SH_Slider_AbsoluteSetButtons)
    {
      return Qt::LeftButton;
    }
    return QProxyStyle::styleHint(hint, option, widget, returnData);
  }
};

Slider::Slider(Qt::Orientation orientation, QWidget* parent)
  : QSlider(orientation, parent)
{
  setStyle(new SliderStyle());
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
    {"invalid", QColor(190, 100, 100, 60) },
    {"ready", QColor(80, 179, 249, 116) },
    {"processing", QColor(241, 241, 249, 116) },
    {"succeeded", QColor(66, 180, 69, 116) },
    {"failed", QColor(175, 0, 38, 116)}
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
