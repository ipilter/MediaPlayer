#include "Slider.h"
#include "Utils.h"

#include <QMouseEvent>
#include <QMediaPlayer>
#include <QPainter>
#include <QStyleOption>
#include <QProxyStyle>
#include <QStyleHintReturn>
#include <QRect>
#include <QMargins>
#include <QWheelEvent>

#include <set>

// Color map for different sequence states
const std::map<QString, const QColor> Slider::sColorMap = {
  { "progressBackground", QColor(255, 255, 255, 85) },
  { "progressStart",      QColor(34, 115, 211, 185) },
  { "progressEnd",        QColor(34, 115, 211, 185)},
  { "invalid",            QColor(255, 0, 0, 155) },
  { "ready",              QColor(255, 255, 255, 55) },
  { "processing",         QColor(255, 255, 255, 155) },
  { "succeeded",          QColor(40, 185,  70, 155) },
  { "failed",             QColor(255,  20,  78, 155) },
  { "selected",           QColor(246, 249,  38, 155) },
  { "editing",            QColor(55, 150, 150, 155) }
  };

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
    const QPoint wClickPos = wEvent->pos();
    // two areas: top half is the seqence area, bottom half is the progress bar area
    const QRect wSequenceArea(0, mSequenceRectBottom, size().width(), mSequenceRectTop);
    // use some threshold to help selecting small pieces
    const QRect wClickArea(wClickPos.x() - (mSequenceRectMinLength / 2), wClickPos.y(), mSequenceRectMinLength, wClickPos.y());
    // check if the click is in the sequence area    
    wHandled = wSequenceArea.intersects(wClickArea);

    // collect intersecting sequences, if any
    OrderedSequenceEntries wIntersectingSequence;
    for (auto& wSequenceEntry : mSequences)
    {
      if (sequenceRect(wSequenceEntry).contains(wClickPos))
      {
        qDebug() << wClickPos << " clicked on " << wSequenceEntry.first.first.toString() << " - " << wSequenceEntry.first.second.toString();
        wIntersectingSequence.insert(&wSequenceEntry);
      }

      // if the click is in the sequence area, toggle selection
      if (wHandled)
      {
        wSequenceEntry.second.mSelected = false;
      }
    }

    // use intersecting sequences using slection logic TODO: to do
    if (wIntersectingSequence.empty())
    {
      emit sequenceSelected(nullptr);
    }
    else
    {
      SequenceEntry* wSelectedSequence = *wIntersectingSequence.begin();

      wSelectedSequence->second.mSelected = true;
      emit sequenceSelected(&wSelectedSequence->first);
      wHandled = true;
    }
    update();

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

void Slider::mouseDoubleClickEvent(QMouseEvent* wEvent)
{
  if (wEvent->button() == Qt::LeftButton)
  {
    const QPoint wClickPos = wEvent->pos();
    OrderedSequenceEntries wIntersectingSequence;
    for (auto& wSequenceEntry : mSequences)
    {
      if (sequenceRect(wSequenceEntry).contains(wClickPos))
      {
        qDebug() << wClickPos << " double clicked on " << wSequenceEntry.first.first.toString() << " - " << wSequenceEntry.first.second.toString();
        wIntersectingSequence.insert(&wSequenceEntry);
      }
    }
    if (!wIntersectingSequence.empty())
    {
      SequenceEntry* wSelectedSequence = *wIntersectingSequence.begin();
      emit sequenceDoubleClicked(&wSelectedSequence->first);
    }
  }
}

void Slider::paintEvent(QPaintEvent* wEvent)
{
  QSlider::paintEvent(wEvent);

  OrderedSequenceEntries wOrderedSequences;
  for (auto& wSequenceEntry : mSequences)
  {
    wOrderedSequences.insert(&wSequenceEntry);
  }

  QPainter wPainter(this);
  auto wIt = wOrderedSequences.rbegin();
  while (wIt != wOrderedSequences.rend())
  {
    const SequenceEntry& wSequenceEntry = **wIt;
    QRect wFullRect = sequenceRect(wSequenceEntry);
    if(wSequenceEntry.second.mState != OperationState::Processing)
    {
      wPainter.fillRect(wFullRect, sequenceColor(wSequenceEntry));
    }
    else
    {
      const VTime wDuration = wSequenceEntry.first.second - wSequenceEntry.first.first;
      const VTime wProcessTime = wSequenceEntry.second.mProcessTimer;

      const int wStartX = QStyle::sliderPositionFromValue(minimum(), maximum(), static_cast<int>(wSequenceEntry.first.first.ms()), width());
      const int wProcessEndX = QStyle::sliderPositionFromValue(minimum(), maximum(), static_cast<int>(wSequenceEntry.first.first.ms() + wProcessTime.ms()), width());
      const int wRectY = mSequenceRectBottom;
      const int wRectH = mSequenceRectTop;  

      const QRect wProcessedRect(wStartX, wRectY, wProcessEndX - wStartX, wRectH);
      const QRect wNonProcessedRect(wProcessEndX, wRectY, wFullRect.right() - wProcessEndX + 1, wRectH);

      double wT = 0.0;
      if (wDuration.ms() > 0) {
        wT = std::min(1.0, std::max(0.0, static_cast<double>(wProcessTime.ms()) / static_cast<double>(wDuration.ms())));
      }

      wPainter.fillRect(wProcessedRect, utils::lerp(sColorMap.at("progressStart"), sColorMap.at("progressEnd"), wT));
      wPainter.fillRect(wNonProcessedRect, sColorMap.at("progressBackground"));
    }

    ++wIt;
  }
}

void Slider::setSequences(const SequenceMap& wSequence)
{
  mSequences.clear();
  for (auto& wSequence : wSequence)
  {
    mSequences[wSequence.first] = SequenceState{ wSequence.second };
  }
  update();
  emit sequenceSelected(nullptr);
}

const QColor& Slider::sequenceColor(const SequenceEntry& wSequenceEntry) const
{
  QString colorName = "invalid";
  if (wSequenceEntry.second.mIsEditing)
  {
    colorName = "editing";
  }
  else if (wSequenceEntry.second.mSelected)
  {
    colorName = "selected";
  }
  else
  {
    switch (wSequenceEntry.second.mState)
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
  return sColorMap.at(colorName);
}

const QRect Slider::sequenceRect(const SequenceEntry& wSequenceEntry) const
{
  int wStartX = QStyle::sliderPositionFromValue(minimum(), maximum(), static_cast<int>(wSequenceEntry.first.first.ms()), width());
  int wEndX = QStyle::sliderPositionFromValue(minimum(), maximum(), static_cast<int>(wSequenceEntry.first.second.ms()), width());

  int wLength = wEndX - wStartX;
  if (wLength < mSequenceRectMinLength)
  {
    wEndX += mSequenceRectMinLength - wLength;
    wLength = wEndX - wStartX;
  }

  return QRect(wStartX, mSequenceRectBottom, wLength, mSequenceRectTop);
}

void Slider::wheelEvent(QWheelEvent* wEvent)
{
  // TODO move this logic out form the UI element, see MainWindow::keyPressEvent
  int stepMs = 500;
  const auto mods = wEvent->modifiers();
  if (mods & Qt::ControlModifier)
    stepMs = 25;
  else if (mods & Qt::ShiftModifier)
    stepMs = 5'000;

  const int numSteps = wEvent->angleDelta().y() / 120;
  if (numSteps == 0) {
    wEvent->ignore();
    return;
  }

  int newValue = value() + numSteps * stepMs;
  newValue = qBound(minimum(), newValue, maximum());
  setValue(newValue);
  emit sliderMoved(newValue);

  wEvent->accept();
}
