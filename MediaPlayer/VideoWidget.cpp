#include "VideoWidget.h"
#include <QMouseEvent>
#include <QMimeData>
#include <QFileInfo>

VideoWidget::VideoWidget(QWidget* parent)
  : QVideoWidget(parent)
{
  setObjectName("videoWidget");
  setAcceptDrops(true);
}

void VideoWidget::mousePressEvent(QMouseEvent* event)
{
  if (event->button() == Qt::LeftButton)
  {
    emit mouseClicked();
  }
  QVideoWidget::mousePressEvent(event);
}

void VideoWidget::dragEnterEvent(QDragEnterEvent* event)
{
  if (event->mimeData()->hasUrls())
  {
    event->acceptProposedAction();
  }
}

void VideoWidget::dropEvent(QDropEvent* event)
{
  if (event->mimeData()->hasUrls())
  {
    emit filesDropped(event->mimeData()->urls());
  }
}
