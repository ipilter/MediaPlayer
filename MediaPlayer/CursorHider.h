#pragma once

#include <QObject>

class QTimer;
class QEvent;
class QWidget;

class CursorHider : public QObject
{
  Q_OBJECT

public:
  CursorHider(QWidget* target);
  ~CursorHider();

  void setTimeout(int timeoutMs);

protected:
  bool eventFilter(QObject* obj, QEvent* event) override;

private slots:
  void hideCursor();

private:
  QWidget* mTarget;
  QTimer* mTimer;
  int mTimeoutMs = 1000;
  bool mCursorHidden;
};
