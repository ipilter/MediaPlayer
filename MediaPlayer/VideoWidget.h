#pragma once

#include <QVideoWidget>
#include <QDropEvent>
#include <QDragEnterEvent>

class VideoWidget : public QVideoWidget
{
  Q_OBJECT

public:
  explicit VideoWidget(QWidget* parent = nullptr);
  ~VideoWidget() override = default;

signals:
  void mouseClicked();
  void filesDropped(const QList<QUrl>& urls);

protected:
  virtual void mousePressEvent(QMouseEvent* event) override;
  virtual void dragEnterEvent(QDragEnterEvent* event) override;
  //virtual void dragMoveEvent(QDragMoveEvent *event) override;
  //virtual void dragLeaveEvent(QDragLeaveEvent *event) override;
  virtual void dropEvent(QDropEvent* event) override;
};
