#include "Slider.h"

#include <QMouseEvent>
#include <QMediaPlayer>
#include <QPainter>
#include <QStyleOption>
#include <QProxyStyle>
#include <QStyleHintReturn>
#include <QRect>
#include <QMargins>

// Custom style for the slider
class SliderStyle : public QProxyStyle
{
public:
  SliderStyle() : QProxyStyle() {}

  int styleHint(StyleHint wHint, const QStyleOption* wOption, const QWidget* wWidget, QStyleHintReturn* wReturnData) const override
  {
    if (wHint == QStyle::SH_Slider_AbsoluteSetButtons)
    {
      return Qt::LeftButton;
    }
    return QProxyStyle::styleHint(wHint, wOption, wWidget, wReturnData);
  }
};

// Slider class implementation
Slider::Slider(Qt::Orientation wOrientation, QWidget* wParent)
  : QSlider(wOrientation, wParent)
{
  setStyle(new SliderStyle());
}

void Slider::mousePressEvent(QMouseEvent* wEvent)
{
  bool wHandled = false;

  if (wEvent->button() == Qt::LeftButton)
  {
    for (auto& wSequence : mSequences)
    {
      wSequence.second.mSelected = false;
    }

    const QPoint clickPos = wEvent->pos();
    for (auto& wSequence : mSequences)
    {
      if (sequenceRect(wSequence).contains(clickPos))
      {
        qDebug() << clickPos << " clicked on " << wSequence.first.first.toString() << " - " << wSequence.first.second.toString();
        wSequence.second.mSelected = true;
        update();
        wHandled = true;
      }
    }

    if (!wHandled)
    {
      const int value = QStyle::sliderValueFromPosition(minimum(), maximum(), wEvent->pos().x(), width());
      setValue(value);
      emit sliderMoved(value);
    }
  }

  if (!wHandled)
  {
    QSlider::mousePressEvent(wEvent);
  }
  else
  {
    wEvent->accept();
  }
}

void Slider::paintEvent(QPaintEvent* wEvent)
{
  QSlider::paintEvent(wEvent);

  QPainter wPainter(this);  
  for (const auto& wSequence : mSequences)
  {
    wPainter.fillRect(sequenceRect(wSequence), sequenceColor(wSequence));
  }
}

void Slider::setSequences(const SequenceMap& wSequence)
{
  mSequences.clear();
  for (auto& wSequence : wSequence)
  {
    mSequences[wSequence.first] = SequenceState{ wSequence.second.mState, wSequence.second.mSelected, wSequence.second.mIsEditing };
  }
  update();
}


const QColor& Slider::sequenceColor(const SequenceEntry& wSequence) const
{
  static const std::map<QString, const QColor> colorMap = {
    {"invalid",    QColor(255, 100, 100, 210) },
    {"ready",      QColor(80, 120, 255, 156)  },
    {"processing", QColor(255, 255, 255, 156) },
    {"succeeded",  QColor(80, 250, 90, 156)   },
    {"failed",     QColor(255, 20, 78, 156)    },
    {"selected",   QColor(255, 255, 155, 156) },
    {"editing",    QColor(155, 200, 210, 156) }
  };

  QString colorName = "invalid";

  // if editing or selected
  if (wSequence.second.mIsEditing)
  {
    colorName = "editing";
  }
  else if (wSequence.second.mSelected)
  {
    colorName = "selected";
  }
  else
  {
    // if alredy done, color by cut operation state
    switch (wSequence.second.mState)
    {
      case OperationState::Ready:
      colorName = "ready";
      break;
      case OperationState::Processing:
      colorName = "processing";
      break;
      case OperationState::Succeeded:
      colorName = "succeeded";
      break;
      case OperationState::Failed:
      colorName = "failed";
      break;
    }
  }
  return colorMap.at(colorName);
}

const QRect Slider::sequenceRect(const SequenceEntry& wSequence) const
{
  int wStartX = QStyle::sliderPositionFromValue(minimum(), maximum(), static_cast<int>(wSequence.first.first.ms()), width());
  int wEndX = QStyle::sliderPositionFromValue(minimum(), maximum(), static_cast<int>(wSequence.first.second.ms()), width());

  int wLength = wEndX - wStartX;
  if (wLength < mSequenceRectMinLength)
  {
    wEndX += mSequenceRectMinLength - wLength;
    wLength = wEndX - wStartX;
  }

  return QRect(wStartX, mSequenceRectBottom, wLength, mSequenceRectTop);
}
