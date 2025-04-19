#pragma once

#include "Types.h"

#include <QSlider>

class Slider : public QSlider
{
  Q_OBJECT

public:
  explicit Slider(Qt::Orientation orientation, QWidget* parent = nullptr);

  void setSequences(const Sequences& sequences);

protected:
  void paintEvent(QPaintEvent* event) override;
  void mousePressEvent(QMouseEvent* event) override;

private:
  Sequences mSequences;
};
