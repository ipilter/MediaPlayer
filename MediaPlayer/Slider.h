#pragma once

#include "Types.h"

#include <QSlider>

class Slider : public QSlider
{
  Q_OBJECT

public:
  explicit Slider(Qt::Orientation orientation, QWidget* parent = nullptr);

  void addSequence(const Sequence& sequence);

protected:
  void paintEvent(QPaintEvent* event) override;
  void mousePressEvent(QMouseEvent* event) override;

  Sequences mSequences;
};
