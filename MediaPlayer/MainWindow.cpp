#include "MainWindow.h"
#include "View.h"
#include "MediaPlayer.h"
#include "Settings.h"

#include <QTime>
#include <QFile>
#include <QKeyEvent>
#include <QString>

MainWindow::MainWindow(QWidget* parent)
  : QMainWindow(parent)
  , mMediaPlayer(std::make_shared<MediaPlayer>()) // TODO: createo out of main window, best in main, destructor crash in view class !
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
    {
      close();
      break;
    }
    case Qt::Key_1:
    {
      if (event->modifiers() & Qt::ShiftModifier)
      {
        qDebug() << "ShiftModifier + 1 pressed!";
      }
      else if (event->modifiers() & Qt::AltModifier)
      {
        qDebug() << "AltModifier + 1 pressed!";
      }
      else if (event->modifiers() & Qt::ControlModifier)
      {
        qDebug() << "ControlModifier + 1 pressed!";
      }
      else
      {
        qDebug() << "1 with No Modifier pressed!";
        mMediaPlayer->deleteCurrentVideo();
      } 
      break;
    }
    case Qt::Key_2:
    {
      if (event->modifiers() & Qt::ShiftModifier)
      {
      }
      else if (event->modifiers() & Qt::AltModifier)
      {
      }
      else if (event->modifiers() & Qt::ControlModifier)
      {
      }
      else
      {
        mMediaPlayer->deleteSequence();
      }
      break;
    }
    case Qt::Key_A:
    {
      MediaPlayer::SeekStep step = MediaPlayer::SeekStep::Normal;
      if (event->modifiers() & Qt::ShiftModifier)
      {
        step = MediaPlayer::SeekStep::Big;
      }
      else if (event->modifiers() & Qt::ControlModifier)
      {
        step = MediaPlayer::SeekStep::Small;
      }
      else if (event->modifiers() & Qt::AltModifier)
      {
        mMediaPlayer->snapToSelection(MediaPlayer::SnapPosition::Start);
        break;
      }
      mMediaPlayer->seek(MediaPlayer::SeekDirection::Backward, step);
      break;
    }
    case Qt::Key_D:
    {
      MediaPlayer::SeekStep step = MediaPlayer::SeekStep::Normal;
      if (event->modifiers() & Qt::ShiftModifier)
      {
        step = MediaPlayer::SeekStep::Big;
      }
      else if (event->modifiers() & Qt::ControlModifier)
      {
        step = MediaPlayer::SeekStep::Small;
      }
      else if (event->modifiers() & Qt::AltModifier)
      {
        mMediaPlayer->snapToSelection(MediaPlayer::SnapPosition::End);
        break;
      }
      mMediaPlayer->seek(MediaPlayer::SeekDirection::Forward, step);
      break;
    }
    case Qt::Key_C:
    {
      if (event->modifiers() & Qt::ControlModifier)
      {
      }
      else if (event->modifiers() & Qt::ShiftModifier)
      {
      }
      else if (event->modifiers() & Qt::AltModifier)
      {
      }
      else
      {
        mMediaPlayer->next();
      }
      break;
    }
    case Qt::Key_W:
    {
      if (event->modifiers() & Qt::ControlModifier)
      {
      }
      else if (event->modifiers() & Qt::ShiftModifier)
      {
        mMediaPlayer->mark(true);
      }
      else if (event->modifiers() & Qt::AltModifier)
      {
      }
      else
      {
        mMediaPlayer->mark();
      }
      break;
    }
    case Qt::Key_X:
    {
      if (event->modifiers() & Qt::ControlModifier)
      {
      }
      else if (event->modifiers() & Qt::ShiftModifier)
      {
        mMediaPlayer->seek(MediaPlayer::SeekDirection::Backward, MediaPlayer::SeekStep::Random);
      }
      else if (event->modifiers() & Qt::AltModifier)
      {
      }
      else
      {
        mMediaPlayer->seek(MediaPlayer::SeekDirection::Forward, MediaPlayer::SeekStep::Random);
      }
      break;
    }
    case Qt::Key_S:
    {
      break;
    }
    case Qt::Key_Z:
    {
      if (event->modifiers() & Qt::ControlModifier)
      {
      }
      else if (event->modifiers() & Qt::ShiftModifier)
      {
      }
      else if (event->modifiers() & Qt::AltModifier)
      {
      }
      else
      {
        mMediaPlayer->previous();
      }
      break;
    }
    case Qt::Key_Space:
    {
      mMediaPlayer->startStop();
      break;
    }
    case Qt::Key_F:
    {
      if (this->isMaximized())
      {
        this->showNormal();
      }
      else
      {
        this->showMaximized();
        //TODO send event to view to hide mouse cursor if needed. at least schedule the timer to hide the cursor
      }
      break;
    }
    case Qt::Key_P:
    {
      mMediaPlayer->setSettings({ !mMediaPlayer->getSettings().mAutoPlay, mMediaPlayer->getSettings().mAudioMode, mMediaPlayer->getSettings().mShowFirstFrame });
      break;
    }
    case Qt::Key_R:
    {
      if (event->modifiers() & Qt::ControlModifier)
      {
        mMediaPlayer->resetSeqenceState();
      }
      else if (event->modifiers() & Qt::ShiftModifier)
      {
      }
      else if (event->modifiers() & Qt::AltModifier)
      {
      }
      else
      {
        mMediaPlayer->previous();
      }
      break;
    }
    case Qt::Key_V:
    {
      MediaPlayer::CutMethod wCutMethod = MediaPlayer::CutMethod::Fast;
      if (event->modifiers() & Qt::ShiftModifier)
      {
        wCutMethod = MediaPlayer::CutMethod::Precise;
      }
      else if (event->modifiers() & Qt::AltModifier)
      {
      }
      else if (event->modifiers() & Qt::ControlModifier)
      {
        wCutMethod = MediaPlayer::CutMethod::Loop;
      }
      mMediaPlayer->cut(wCutMethod);
      break;
    }
    case Qt::Key::Key_Delete:
    {
      if (event->modifiers() & Qt::ShiftModifier)
      {
      }
      else if (event->modifiers() & Qt::AltModifier)
      {
      }
      else if (event->modifiers() & Qt::ControlModifier)
      {
      }
      break;
    }
    default:
    {
      QMainWindow::keyPressEvent(event);
      break;
    }
  }
}

void MainWindow::resizeEvent(QResizeEvent* event)
{
  if (!isMaximized())
  {
    mPlacement.mSize = event->size();
  }  
  QMainWindow::resizeEvent(event);
}

void MainWindow::moveEvent(QMoveEvent* event)
{
  if (!isMaximized())
  {
    mPlacement.mPosition = event->pos();
  }
  QMainWindow::moveEvent(event);
}

void MainWindow::closeEvent(QCloseEvent* event)
{
  QMainWindow::closeEvent(event);
  QApplication::quit();
}

void MainWindow::setPlaylist(const Playlist& playlist)
{
  mMediaPlayer->setPlaylist(playlist);
}

void MainWindow::setSettings(const Settings& settings)
{
  mMediaPlayer->setSettings(settings);
}

const Settings& MainWindow::getSettings() const
{
  return mMediaPlayer->getSettings();
}

const Placement& MainWindow::getPlacement() const
{
  return mPlacement;
}
