#include "MainWindow.h"
#include "View.h"
#include "MediaPlayer.h"

#include <QTime>
#include <QFile>
#include <QKeyEvent>
#include <QString>

MainWindow::MainWindow(QWidget* parent)
  : QMainWindow(parent)
  , mMediaPlayer(std::make_shared<MediaPlayer>(this))
{
  ui.setupUi(this);

  QWidget* wCentralWidget = centralWidget();
  if (wCentralWidget == nullptr)
  {
    wCentralWidget = new QWidget(this);
    setCentralWidget(wCentralWidget);
  }
  wCentralWidget->setLayout(mMediaPlayer->getLayout());
}

MainWindow::~MainWindow()
{}

void MainWindow::keyPressEvent(QKeyEvent* event)
{
  switch (event->key())
  {
  case Qt::Key_Escape:
    QApplication::quit();
    break;
  case Qt::Key_A:
  {
    VTime size = 500;
    if (event->modifiers() & Qt::ShiftModifier)
    {
      size = 5000;
    }
    else if (event->modifiers() & Qt::ControlModifier)
    {
      size = 50;
    }
    mMediaPlayer->seekBackward(size);
    break;
  }
  case Qt::Key_D:
  {
    VTime size = 500;
    if (event->modifiers() & Qt::ShiftModifier)
    {
      size = 5000;
    }
    else if (event->modifiers() & Qt::ControlModifier)
    {
      size = 50;
    }
    mMediaPlayer->seekForward(size);
    break;
  }
  case Qt::Key_C:
  {
    const bool wReconvert = event->modifiers() & Qt::ShiftModifier;
    mMediaPlayer->cut(wReconvert);
    break;
  }
  case Qt::Key_W:
    if (event->modifiers() & Qt::ShiftModifier)
    {
      mMediaPlayer->cancelMark();
    }
    else
    {
      mMediaPlayer->mark();
    }
    break;
  case Qt::Key_N:
    mMediaPlayer->next();
    break;
  case Qt::Key_B:
    mMediaPlayer->previous();
    break;
  case Qt::Key_Space:
    mMediaPlayer->startStop();
    break;
  case Qt::Key_F:
    {
      if (this->isMaximized())
      {
        this->showNormal();
      }
      else
      {
        this->showMaximized();
      }
    }
    break;
  case Qt::Key_P:
    mMediaPlayer->setSettings({ !mMediaPlayer->getSettings().mAutoPlay, mMediaPlayer->getSettings().mMuted});
    break;
  default:
    QMainWindow::keyPressEvent(event);
    break;
  }
}

void MainWindow::setPlaylist(const Playlist& playlist)
{
  mMediaPlayer->setPlaylist(playlist);
}

void MainWindow::setSettings(const MediaPlayer::Settings& settings)
{
  mMediaPlayer->setSettings(settings);
}

const MediaPlayer::Settings& MainWindow::getSettings() const
{
  return mMediaPlayer->getSettings();
}
