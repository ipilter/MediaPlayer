#include "MediaPlayer.h"
#include "VideoPlayer.h"
#include "View.h"
#include "PreciseCutter.h"
#include "FastCutter.h"
#include "Reverser.h"
#include "Merger.h"
#include "ProcessTree.h"
#include "Utils.h"

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
    emit sequencesChanged(mSequenceMap);  // TODO: store the sequences associated to the video, ...

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
    mEditedSequence.second = 1; // to alloqw sequences start with 0..
  }
  else
  {
    mEditedSequence.second = mPlayer->getPosition();

    if (mEditedSequence.second - mEditedSequence.first < 1 ) 
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

void MediaPlayer::logStatusMessage(const QString& msg)
{
  mView->setInfo(msg);
}

void MediaPlayer::cut(const CutMethod cutMethod)
{
  const QString& wVideoPath = mPlaylist[mCurrentVideo].toLocalFile();

  const QString wOutputRootDirectory = "e:\\";

  for (auto& wSequenceEntry : mSequenceMap)
  {
    if (wSequenceEntry.second != SequenceState::Ready)
    {
      continue;
    }

    //wSequence.second = SequenceState::Processing;
    //emit sequencesChanged(mSequenceMap);

    switch (cutMethod)
    {
      case CutMethod::Fast:
        FastCut(wSequenceEntry, wVideoPath, wOutputRootDirectory);
      break;
      case CutMethod::Precise:
        PreciseCut(wSequenceEntry, wVideoPath, wOutputRootDirectory);
      break;
      case CutMethod::Loop:
        LoopCut(wSequenceEntry, wVideoPath, wOutputRootDirectory, 3);
      break;
    }
  }
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

void MediaPlayer::FastCut(SequenceEntry& sequenceEntry, const QString& videoPath, const QString& outputRootDirectory)
{
  const VTime wStartTime = sequenceEntry.first.first;
  const VTime wEndTime = sequenceEntry.first.second;
  const QString wCutFilePath = getNewFileName(QString("%1_%2.mp4")
    .arg(outputRootDirectory + QFileInfo(videoPath).completeBaseName() + "." + wStartTime.toString('-')).arg((wEndTime - wStartTime).seconds()));

  FastCutter::Ptr wCutter = FastCutter::create(videoPath, wCutFilePath, wStartTime, wEndTime);

  connect(wCutter.get(), SIGNAL(Runnable::logMessageEvent), this, SLOT(MainWindow::onLogMessageEvent));

  mCutProcesses.push_back(ProcessTree::create(
    std::move(wCutter),
    [ & ](const QString& msg)
  {
    sequenceEntry.second = SequenceState::Processing;
    emit sequencesChanged(mSequenceMap);
    logStatusMessage(msg);
  },
    [ & ](const QString& msg)
  {
    sequenceEntry.second = SequenceState::Succeeded;
    emit sequencesChanged(mSequenceMap);
    logStatusMessage(msg);
  }));

  // kick in the process
  mCutProcesses.back()->start();
}

void MediaPlayer::PreciseCut(SequenceEntry& sequenceEntry, const QString& videoPath, const QString& outputRootDirectory)
{
  const VTime wStartTime = sequenceEntry.first.first;
  const VTime wEndTime = sequenceEntry.first.second;
  const QString wCutFilePath = getNewFileName(QString("%1_%2.mp4")
    .arg(outputRootDirectory + QFileInfo(videoPath).completeBaseName() + "." + wStartTime.toString('-')).arg((wEndTime - wStartTime).seconds()));

  PreciseCutter::Ptr wCutter = PreciseCutter::create(videoPath, wCutFilePath, wStartTime, wEndTime);

  connect(wCutter.get(), SIGNAL(Runnable::logMessageEvent), this, SLOT(MainWindow::onLogMessageEvent));

  mCutProcesses.push_back(ProcessTree::create(
    std::move(wCutter),
    [ & ](const QString& msg)
  {
    sequenceEntry.second = SequenceState::Processing;
    emit sequencesChanged(mSequenceMap);
    logStatusMessage(msg);
  },
    [ & ](const QString& msg)
  {
    sequenceEntry.second = SequenceState::Succeeded;
    emit sequencesChanged(mSequenceMap);
    logStatusMessage(msg);
  }));

  // kick in the process
  mCutProcesses.back()->start();
}

void MediaPlayer::LoopCut(SequenceEntry& sequenceEntry, const QString& videoPath, const QString& outputRootDirectory, int loopCount)
{
  const VTime wStartTime = sequenceEntry.first.first;
  const VTime wEndTime = sequenceEntry.first.second;
  const QString wCutFilePath = getNewFileName(QString("%1_%2.mp4")
    .arg(outputRootDirectory + QFileInfo(videoPath).completeBaseName() + "." + wStartTime.toString('-')).arg((wEndTime - wStartTime).seconds()));

  // cut first using precise method
  PreciseCutter::Ptr wCutter = PreciseCutter::create(videoPath, wCutFilePath, wStartTime, wEndTime);

  connect(wCutter.get(), SIGNAL(Runnable::logMessageEvent), this, SLOT(MainWindow::onLogMessageEvent));

  mCutProcesses.push_back(ProcessTree::create(
    std::move(wCutter),
    [ & ](const QString& msg)
  {
    sequenceEntry.second = SequenceState::Processing;
    emit sequencesChanged(mSequenceMap);
    logStatusMessage(msg);
  },
    [ & ](const QString& msg)
  {
    logStatusMessage(msg);
  }));

  // reverse the video into an intermediate file _reversed.mp4
  const QString reversedFilePath = getNewFileName(wCutFilePath + "_reversed.mp4");
  const QString mergedFilePath = getNewFileName(outputRootDirectory + QFileInfo(videoPath).completeBaseName() + ".loop.mp4");  // TODO safer !! 

  ProcessTree::Ptr& pReverseProcess =
    mCutProcesses.back()->addChild(Reverser::create(
      wCutFilePath, reversedFilePath),
      [ & ](const QString& msg) { logStatusMessage(msg); },
      [ & ](const QString& msg) { logStatusMessage(msg); });

  // merge the cut and the reversed video into a new file N times
  ProcessTree::Ptr& pMergeProcess =
    pReverseProcess->addChild(Merger::create(
      wCutFilePath, reversedFilePath, mergedFilePath, loopCount),
      [ & ](const QString& msg) 
  {
    logStatusMessage(msg);
  },
      [ & ](const QString& msg) 
  {
    sequenceEntry.second = SequenceState::Succeeded;
    emit sequencesChanged(mSequenceMap);
    logStatusMessage(msg);
  }
    );

  // kick in the process chain
  mCutProcesses.back()->start();
}

void MediaPlayer::resetSeqenceState()
{
  for(auto& wSequence : mSequenceMap)
  {
    wSequence.second = SequenceState::Ready;
  }
  emit sequencesChanged(mSequenceMap);
}

void MediaPlayer::popLastSequence()
{
  if (mSequenceMap.empty())
  {
    return;
  }

  auto wLastSequence = mSequenceMap.end();
  --wLastSequence;

  mSequenceMap.erase(wLastSequence);
  emit sequencesChanged(mSequenceMap);
}
