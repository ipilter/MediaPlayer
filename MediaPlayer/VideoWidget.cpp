#include "VideoWidget.h"

#include <QMouseEvent>

VideoWidget::VideoWidget(QWidget* parent)
  : QVideoWidget(parent)
{
  setObjectName("videoWidget");
}

void VideoWidget::mousePressEvent(QMouseEvent* event)
{
  if (event->button() == Qt::LeftButton)
  {
    emit mouseClicked();
  }
  QVideoWidget::mousePressEvent(event);
}
