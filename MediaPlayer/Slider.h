#pragma once

#include "Types.h"

#include <QSlider>

class Slider : public QSlider
{
  Q_OBJECT

public:
  explicit Slider(Qt::Orientation wOrientation, QWidget* wParent = nullptr);

  void setSequences(const SequenceMap& wSequences);

protected:
  void paintEvent(QPaintEvent* wEvent) override;
  void mousePressEvent(QMouseEvent* wEvent) override;

private:
  const QColor& sequenceColor(const SequenceEntry& wSequence) const;
  const QRect sequenceRect(const SequenceEntry& wSequence) const;

  SequenceMap mSequences;

  const int mSequenceRectBottom = 2;
  const int mSequenceRectTop = 10;
  const int mSequenceRectMinLength = 3;
};
