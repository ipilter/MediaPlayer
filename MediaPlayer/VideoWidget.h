#pragma once

#include <QVideoWidget>

class VideoWidget : public QVideoWidget
{
  Q_OBJECT

public:
  explicit VideoWidget(QWidget* parent = nullptr);
  ~VideoWidget() override = default;

signals:
  void mouseClicked();

protected:
    virtual void mousePressEvent(QMouseEvent* event) override;
};
