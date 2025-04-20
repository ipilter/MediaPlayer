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
  QObject::connect(mPlayer.get(), &VideoPlayer::positionChanged, this, [this](VTime position) { mView->setPosition(position); });
  QObject::connect(mPlayer.get(), &VideoPlayer::durationChanged, this, [this](VTime duration) { mView->setDuration(duration); });
  QObject::connect(mPlayer.get(), &VideoPlayer::videoLoaded,     this, &MediaPlayer::onVideoLoaded);
  QObject::connect(mPlayer.get(), &VideoPlayer::videoEnded,      this, &MediaPlayer::onVideoEnded); // TODO: add settings for loop,next,stop

  QObject::connect(mView.get(), &View::onMouseClick,           this, [this]() { mView->hide(); });
  QObject::connect(mView.get(), &View::sliderChanged,          this, [this](int position) {setPosition(static_cast<VTime>(position)); });
  QObject::connect(mView.get(), &View::previousButtonClicked,  this, [this]() { previous(); });
  QObject::connect(mView.get(), &View::startStopButtonClicked, this, [this]() { startStop(); });
  QObject::connect(mView.get(), &View::nextButtonClicked,      this, [this]() { next(); });
  QObject::connect(mView.get(), &View::muteButtonClicked,      this, [this]() { toggleMute(); });

  QObject::connect(this, &MediaPlayer::sequencesChanged, mView.get(), &View::onSequencesChanged);

  mPlayer->setVolume(50);

  mEditedSequence = Sequence{ 0, 0 };
}

MediaPlayer::~MediaPlayer()
{}

void MediaPlayer::setPlaylist(const Playlist& playlist)
{
  mPlaylist = playlist;
  mCurrentVideo = 0;
  mPlayer->setVideo(mPlaylist[mCurrentVideo]);

  mSequenceMap.clear();
  emit sequencesChanged(mSequenceMap);  // TODO: store the sequences associated to the video, not the player, so that we can have different sequences for each video in the playlist
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
  return mPlaying;
}

void MediaPlayer::play()
{
  mPlayer->play();
  mView->onPlay(); // TODO: emit ??
  mPlaying = true;

  SetThreadExecutionState(ES_CONTINUOUS | ES_DISPLAY_REQUIRED | ES_SYSTEM_REQUIRED);
}

void MediaPlayer::pause()
{
  mPlayer->pause();
  mPlaying = false;
  
  mView->onPause(); // TODO: emit ??
  
  SetThreadExecutionState(ES_CONTINUOUS);
}

void MediaPlayer::stop()
{
  mPlayer->stop();
  mView->onStop(); // TODO: emit ??
  mPlaying = false;

  SetThreadExecutionState(ES_CONTINUOUS);
}

void MediaPlayer::next()
{
  const bool isPlaying = mPlaying;

  if (mPlaylist.size() == 1)
  {
    setPosition(0);
  }
  else
  {
    if (mCurrentVideo == mPlaylist.size() - 1)
    {
      mCurrentVideo = 0;
    }
    else
    {
      ++mCurrentVideo;
    }

    stop();
    mPlayer->setVideo(mPlaylist[mCurrentVideo]);

    mSequenceMap.clear();
    emit sequencesChanged(mSequenceMap);  // TODO: store the sequences associated to the video, not the player, so that we can have different sequences for each video in the playlist

    // TODO inside view
    // todo own method instead, see onMediaLoaded
    // todo mPlayer->getMetadata() is still the old video here, send  a lambda to the player bove and set the view inside the lambda to make it happen when the video is loaded
    const QFileInfo fileInfo(mPlaylist[mCurrentVideo].toLocalFile());
    //const QString info(fileInfo.completeBaseName() + " - " + QString::number(mPlayer->getMetadata().value(QMediaMetaData::Resolution).value<QSize>().width()) + " x " + QString::number(mPlayer->getMetadata().value(QMediaMetaData::Resolution).value<QSize>().height()));
    const QString info(fileInfo.completeBaseName());
    mView->setInfo(info);
  }

  if (isPlaying)
  {
    play();
  }
}

void MediaPlayer::previous()
{
  const bool isPlaying = mPlayer->isPlaying();

  if (mPlaylist.size() == 1)
  {
    setPosition(0);
  }
  else
  {
    if (mCurrentVideo == 0)
    {
      mCurrentVideo = mPlaylist.size() - 1;
    }
    else
    {
      --mCurrentVideo;
    }

    stop();
    mPlayer->setVideo(mPlaylist[mCurrentVideo]);
  }

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
  if (mEditedSequence == Sequence{0, 0})
  {
    mEditedSequence.first = mPlayer->getPosition();
  }
  else
  {
    mEditedSequence.second = mPlayer->getPosition();

    if (mEditedSequence.second - mEditedSequence.first < 5 ) 
    {
      mEditedSequence = Sequence{ 0, 0 };
      return;
    }

    mSequenceMap[mEditedSequence] = SequenceState::Ready;
    
    mEditedSequence = Sequence{ 0, 0 };
    emit sequencesChanged(mSequenceMap);
  }
}

void MediaPlayer::cancelMark()
{
  mEditedSequence = Sequence{ 0, 0 };
}

void MediaPlayer::cut(const CutMethod cutMethod)
{
  auto logStatusMessage = [this](const QString& msg) {
    qDebug() << msg;
    };  // TODO, progress bar

  const QString& wVideoPath = mPlaylist[mCurrentVideo].toLocalFile();
  const QString wVideoName = QFileInfo(wVideoPath).completeBaseName();

  const QString wOutputRootDirectory = "e:/";

  for (auto& wSequence : mSequenceMap)
  {
    if (wSequence.second != SequenceState::Ready)
    {
      continue;
    }

    wSequence.second = SequenceState::Processing;
    emit sequencesChanged(mSequenceMap);

    const QString wRootPath = "e:\\";

    const VTime wStartTime = wSequence.first.first;
    const VTime wEndTime = wSequence.first.second;

    const QString wCutFilePath = getNewFileName(QString("%1_%2.mp4")
      .arg(wOutputRootDirectory + wVideoName + "." + wStartTime.toString('-')).arg((wEndTime - wStartTime).seconds()));

    switch (cutMethod)
    {
    case CutMethod::Fast:
    {

    }
    break;
    case CutMethod::Precise:
    {

    }
    break;
    case CutMethod::Loop:
    {
      Cutter::Ptr cutter = Cutter::create(
        wVideoPath, wCutFilePath, wStartTime, wEndTime);

      connect(cutter.get(), SIGNAL(Runnable::logMessageEvent), this, SLOT(MainWindow::onLogMessageEvent));

      // TODO ony one process tree for all the cuts? create fire a forget processes here 
      // what happens with then 2nd iteration, the process is already running and we delete the old one and create new cancelling it !
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
      } // end of lambda
      mCutProcess->start();
    }
    break;
    }
  } // sequence loop
}

void MediaPlayer::onVideoLoaded()
{ 
  const QFileInfo fileInfo(mPlaylist[mCurrentVideo].toLocalFile());
  const QString info(fileInfo.completeBaseName() + " - " + QString::number(mPlayer->getMetadata().value(QMediaMetaData::Resolution).value<QSize>().width()) + " x " + QString::number(mPlayer->getMetadata().value(QMediaMetaData::Resolution).value<QSize>().height()));
  mView->setInfo(info);

  setPosition(0);
  //pause(); // TODO: settings to show first frame on open
  if (mSettings.mAutoPlay)
  {
    play();
  }
}

void MediaPlayer::onVideoEnded()
{
  // TODO add loop|next|stop Settings
  next();
  if (isPlaying())
  {
    play();
  }
}
