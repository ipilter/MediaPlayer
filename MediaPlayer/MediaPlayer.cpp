#include "MediaPlayer.h"
#include "VideoPlayer.h"
#include "View.h"

#include "Utils.h"
#include "Cutter.h"
#include "Reverser.h"
#include "Merger.h"
#include "Cleanup.h"
#include "ProcessTree.h"

#include <QFileInfo.h>
#include <QMediaMetadata.h>
#include <QUrl>
#include <QThread>

#include <random>
#include <windows.h>

MediaPlayer::MediaPlayer(QObject* parent)
  : QObject(parent)
  , mView(std::make_shared<View>())
  , mPlayer(std::make_shared<VideoPlayer>(mView->getVideoWidget()))
{
  QObject::connect(mPlayer.get(), &VideoPlayer::positionChanged, this, [this](VTime position) {mView->setPosition(position); });
  QObject::connect(mPlayer.get(), &VideoPlayer::durationChanged, this, [this](VTime duration) {mView->setDuration(duration); });
  QObject::connect(mPlayer.get(), &VideoPlayer::videoLoaded, this, &MediaPlayer::onVideoLoaded);
  QObject::connect(mPlayer.get(), &VideoPlayer::videoEnded, this, &MediaPlayer::onVideoEnded); // TODO: add settings for loop,next,stop

  QObject::connect(mView.get(), &View::onMouseClick,           this, [this]() { mView->hide(); });
  QObject::connect(mView.get(), &View::sliderChanged,          this, [this](int position) {setPosition(static_cast<VTime>(position)); });
  QObject::connect(mView.get(), &View::previousButtonClicked,  this, [this]() { previous(); });
  QObject::connect(mView.get(), &View::startStopButtonClicked, this, [this]() { startStop(); });
  QObject::connect(mView.get(), &View::nextButtonClicked,      this, [this]() { next(); });
  QObject::connect(mView.get(), &View::muteButtonClicked, this, [this]() { toggleMute(); });

  mPlayer->setVolume(50);


}

MediaPlayer::~MediaPlayer()
{}

void MediaPlayer::setPlaylist(const Playlist& playlist)
{
  mPlaylist = playlist;
  mCurrentVideo = 0;
  mPlayer->setVideo(mPlaylist[mCurrentVideo]);
}

QLayout* MediaPlayer::getLayout() const
{
  return mView->getLayout();
}

void MediaPlayer::setSettings(const Settings& settings)
{
  mSettings = settings;
  
  mPlayer->setMuted(mSettings.mMuted);
  mView->setMuted(mSettings.mMuted);
}

const MediaPlayer::Settings& MediaPlayer::getSettings() const
{
  return mSettings;
}

bool MediaPlayer::isPlaying() const
{
  return mPlayer->isPlaying();
}

void MediaPlayer::play()
{
  mPlayer->play();
  
  mView->onPlay(); // TODO: emit ??

  SetThreadExecutionState(ES_CONTINUOUS | ES_DISPLAY_REQUIRED | ES_SYSTEM_REQUIRED);
}

void MediaPlayer::pause()
{
  mPlayer->pause();
  
  mView->onPause(); // TODO: emit ??
  
  SetThreadExecutionState(ES_CONTINUOUS);
}

void MediaPlayer::stop()
{
  mPlayer->stop();
  mView->onStop(); // TODO: emit ??

  SetThreadExecutionState(ES_CONTINUOUS);
}

void MediaPlayer::next()
{
  if (mCurrentVideo == mPlaylist.size() - 1)
  {
    mCurrentVideo = 0;
  }
  else
  {
    ++mCurrentVideo;
  }

  const bool isPlaying = mPlayer->isPlaying();
  stop();
  mPlayer->setVideo(mPlaylist[mCurrentVideo]);


  // TODO inside view
  // todo own method instead, see onMediaLoaded
  // todo mPlayer->getMetadata() is still the old video here, send  a lambda to the player bove and set the view inside the lambda to make it happen when the video is loaded
  const QFileInfo fileInfo(mPlaylist[mCurrentVideo].toLocalFile());
  //const QString info(fileInfo.completeBaseName() + " - " + QString::number(mPlayer->getMetadata().value(QMediaMetaData::Resolution).value<QSize>().width()) + " x " + QString::number(mPlayer->getMetadata().value(QMediaMetaData::Resolution).value<QSize>().height()));
  const QString info(fileInfo.completeBaseName());
  mView->setInfo(info);

  if (isPlaying)
  {
    play();
  }
}

void MediaPlayer::previous()
{
  if (mCurrentVideo == 0)
  {
    mCurrentVideo = mPlaylist.size() - 1;
  }
  else
  {
    --mCurrentVideo;
  }

  const bool isPlaying = mPlayer->isPlaying();
  stop();
  mPlayer->setVideo(mPlaylist[mCurrentVideo]);
  if (isPlaying)
  {
    play();
  }
}

void MediaPlayer::toggleMute()
{
  mSettings.mMuted = !mSettings.mMuted;

  mPlayer->setMuted(mSettings.mMuted);
  mView->setMuted(mSettings.mMuted);
}

void MediaPlayer::setPosition(VTime position)
{
  mPlayer->setPosition(position);
}

void MediaPlayer::seekBackward(VTime size)
{
  mPlayer->seekBackward(size);
}

void MediaPlayer::seekForward(VTime size)
{
  mPlayer->seekForward(size);
}

void MediaPlayer::startStop()
{
  if (isPlaying())
  {
    pause();
  }
  else
  {
    play();
  }
}

void MediaPlayer::mark()
{
  if (mIsMarking)
  {
    mIsMarking = false;
    mSequences.back().second = mPlayer->getPosition();
    if (mSequences.back().first < mSequences.back().second)
    {
      mView->addSequence(mSequences.back());
    }
    else
    {
      mSequences.pop_back();
    }
    return;
  }

  mIsMarking = true;
  mSequences.push_back(std::make_pair(mPlayer->getPosition(), 0)); // use static instance for invalid sequence (min > max)

  // emit update needed to redraw slider, current solution shady..
}

void MediaPlayer::cancelMark()
{
  mIsMarking = false;
  mSequences.pop_back();
}

void MediaPlayer::cut(const bool reconvert)
{
  auto logStatusMessage = [this](const QString& msg) {
    qDebug() << msg;
    };  // TODO, progress bar

  const QString& wVideoPath = mPlaylist[mCurrentVideo].toLocalFile();
  const QString wVideoName = QFileInfo(wVideoPath).completeBaseName();

  const QString wOutputRootDirectory = "e:/";

  for(const auto& wSequence : mSequences)
  {
    if (wSequence.first > wSequence.second)
    {
      continue;
    }

    const QString wRootPath = "e:\\";

    const VTime wStartTime = wSequence.first;
    const VTime wEndTime = wSequence.second;

    const QString wCutFilePath = getNewFileName(QString("%1_%2.mp4")
      .arg(wOutputRootDirectory + wVideoName + "." + wStartTime.toString('-')).arg((wEndTime - wStartTime).seconds()));

    Cutter::Ptr cutter = Cutter::create(
      wVideoPath, wCutFilePath, wStartTime, wEndTime);

    connect(cutter.get(), SIGNAL(Runnable::logMessageEvent), this, SLOT(MainWindow::onLogMessageEvent));

    mCutProcess = ProcessTreeNode::create(
      std::move(cutter),
      [&](const QString& msg) { logStatusMessage(msg); },
      [&](const QString& msg) { logStatusMessage(msg); });
    
    {
      const QString reversedFilePath = getNewFileName(wCutFilePath + "_reversed.mp4");

      const QString mergedFilePath = getNewFileName(wOutputRootDirectory + wVideoName + ".loop.mp4");  // TODO safer !! 

      ProcessTreeNode::Ptr& pReverseProcess =
        mCutProcess->addChild(Reverser::create(
          wCutFilePath, reversedFilePath),
          [&](const QString& msg) { logStatusMessage(msg); },
          [&](const QString& msg) { logStatusMessage(msg); });

      ProcessTreeNode::Ptr& pMergeProcess =
        pReverseProcess->addChild(Merger::create(
          wCutFilePath, reversedFilePath, mergedFilePath, 4/*ui.mLoopCountSpinBox->value()*/),
          [&](const QString& msg) { logStatusMessage(msg); },
          [&](const QString& msg) { logStatusMessage(msg); }
        );

      //pMergeProcess->addChild(Cleanup::create(cutFilePath),
      //    [&](const QString& msg) { logStatusMessage(msg); },
      //    [&](const QString& msg) { logStatusMessage(msg); });

      //pMergeProcess->addChild(Cleanup::create(reversedFilePath),
      //    [&](const QString& msg) { logStatusMessage(msg); },
      //    [&](const
     } // end of lambda

    mCutProcess->start();

  }

  //update sequence view as done during cut
  //mView->clearSequences(); TODO update sequence color by the progress of the cut, when done the sequence is green, of failed it is red, if not on disk then gray.
}

void MediaPlayer::onVideoLoaded()
{ 
  const QFileInfo fileInfo(mPlaylist[mCurrentVideo].toLocalFile());
  const QString info(fileInfo.completeBaseName() + " - " + QString::number(mPlayer->getMetadata().value(QMediaMetaData::Resolution).value<QSize>().width()) + " x " + QString::number(mPlayer->getMetadata().value(QMediaMetaData::Resolution).value<QSize>().height()));
  mView->setInfo(info);


  setPosition(0);
  if (mSettings.mAutoPlay)
  {
    play();
  }
}

void MediaPlayer::onVideoEnded()
{
  // todo add loop|next|stop Settings
  next();
  play();
}
