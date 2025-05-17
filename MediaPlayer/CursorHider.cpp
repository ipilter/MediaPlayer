#include "CursorHider.h"

#include <QTimer>
#include <QEvent>
#include <QCursor>
#include <QWidget>

CursorHider::CursorHider(QWidget* target)
  : QObject(target)
  , mTarget(target)
  , mCursorHidden(false)
{
  mTimer = new QTimer(this);
  mTimer->setSingleShot(true);
  connect(mTimer, &QTimer::timeout, this, &CursorHider::hideCursor);

  mTarget->installEventFilter(this);
  mTarget->setAttribute(Qt::WA_Hover, true);
}

CursorHider::~CursorHider()
{
  mTimer->stop();
  delete mTimer;
  mTarget->removeEventFilter(this);
}

void CursorHider::setTimeout(int timeoutMs)
{
  mTimeoutMs = timeoutMs;
}

bool CursorHider::eventFilter(QObject* obj, QEvent* event)
{
  if (obj == mTarget)
  {
    qDebug() << "CursorHider::eventFilter" << event->type();
    switch (event->type())
    {
      case QEvent::Leave:
      case QEvent::HoverMove:
      case QEvent::MouseMove:
        if (mCursorHidden)
        {
          mTarget->setCursor(Qt::ArrowCursor);
          mCursorHidden = false;
        }
        mTimer->start(mTimeoutMs);
        break;
      default:
        break;
    }
  }
  return QObject::eventFilter(obj, event);
}

void CursorHider::hideCursor()
{
  mTarget->setCursor(Qt::BlankCursor);
  mCursorHidden = true;
}
