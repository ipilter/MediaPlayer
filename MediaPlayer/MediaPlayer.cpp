#include "MediaPlayer.h"
#include "VideoPlayer.h"
#include "View.h"
#include "Utils.h"

#include <QFileInfo.h>
#include <QMediaMetadata.h>
#include <QUrl>
#include <QThread>
#include <QProcess>

#include <random>
#include <fstream>
#include <windows.h>

MediaPlayer::MediaPlayer(QObject* parent)
  : QObject(parent)
  , mView(std::make_shared<View>())
  , mPlayer(std::make_shared<VideoPlayer>(mView->getVideoWidget()))
{
  QObject::connect(mPlayer.get(), &VideoPlayer::positionChanged, this, [this](VTime position) { mView->setPosition(position); });
  QObject::connect(mPlayer.get(), &VideoPlayer::durationChanged, this, [this](VTime duration) { mView->setDuration(duration); });
  QObject::connect(mPlayer.get(), &VideoPlayer::videoLoaded,     this, &MediaPlayer::onVideoLoaded);
  QObject::connect(mPlayer.get(), &VideoPlayer::videoEnded,      this, &MediaPlayer::onVideoEnded);

  QObject::connect(mView.get(), &View::onMouseClick,           this, [this]() { mView->hide(); });
  QObject::connect(mView.get(), &View::sliderChanged,          this, [this](int position) {setPosition(static_cast<VTime>(position)); });
  QObject::connect(mView.get(), &View::previousButtonClicked,  this, [this]() { previous(); });
  QObject::connect(mView.get(), &View::startStopButtonClicked, this, [this]() { startStop(); });
  QObject::connect(mView.get(), &View::nextButtonClicked,      this, [this]() { next(); });
  QObject::connect(mView.get(), &View::muteButtonClicked,      this, [this]() { toggleMute(); });

  QObject::connect(this, &MediaPlayer::sequencesChanged, mView.get(), &View::onSequencesChanged);

  mPlayer->setVolume(50);
  //mPlayer->setPlaybackRate(0.5f);

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

void MediaPlayer::setPosition(const VTime& position)
{
  mPlayer->setPosition(position);
}

void MediaPlayer::seek(MediaPlayer::SeekDirection direction, MediaPlayer::SeekStep step)
{
  VTime stepSize;
  switch (step)
  {
    case SeekStep::Small:
    stepSize = 50;  // TODO use settings
    break;
    case SeekStep::Big:
    stepSize = 5000;
    break;
    default:
    stepSize = 500;
    break;
  }

  if (direction == SeekDirection::Forward)
  {
    mPlayer->seekForward(stepSize);
  }
  else
  {
    mPlayer->seekBackward(stepSize);
  }
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
  for (auto& wSequenceEntry : mSequenceMap)
  {
    if (wSequenceEntry.second != SequenceState::Ready)
    {
      continue;
    }

    switch (cutMethod)
    {
      case CutMethod::Fast:
        FastCut(wSequenceEntry);
      break;
      case CutMethod::Precise:
        PreciseCut(wSequenceEntry);
      break;
      case CutMethod::Loop:
        LoopCut(wSequenceEntry, 3);
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

void MediaPlayer::FastCut(SequenceEntry& sequenceEntry)
{
  const VTime wStartTime = sequenceEntry.first.first;
  const VTime wEndTime = sequenceEntry.first.second;
  const QString& wVideoPath = mPlaylist[mCurrentVideo].toLocalFile();

  const QString wCutFilePath = mOutputRootDirectory + prettifyFileName(QFileInfo(wVideoPath).completeBaseName()) + "." + wStartTime.toString('.') + "." + QString::number((wEndTime - wStartTime).ms()) + ".mp4";
  
  mProcesses.push_back(std::make_unique<QProcess>(this));
  connect(mProcesses.back().get(), &QProcess::started, this, [ & ]() {
    sequenceEntry.second = SequenceState::Processing;
    emit sequencesChanged(mSequenceMap);
    logStatusMessage("Fast cut started");
  });

  connect(mProcesses.back().get(), &QProcess::finished, this, [ & ](int exitCode, QProcess::ExitStatus exitStatus) {
    sequenceEntry.second = exitCode == 0 ? SequenceState::Succeeded : SequenceState::Failed;
    emit sequencesChanged(mSequenceMap);
    logStatusMessage(QString("Fast cut ") + (exitCode == 0 ? "succeeded" : "failed"));
  });

  mProcesses.back()->start(mFFMpegPath, {
    "-ss", wStartTime.toString(),
    "-i", wVideoPath,
    "-t", (wEndTime - wStartTime).toString(),
    "-async", "1",
    "-vcodec", "copy",
    "-acodec", "copy",
    "-avoid_negative_ts", "1",
    wCutFilePath, "-y"});
}

void MediaPlayer::PreciseCut(SequenceEntry& sequenceEntry)
{
  const VTime wStartTime = sequenceEntry.first.first;
  const VTime wEndTime = sequenceEntry.first.second;
  const QString& wVideoPath = mPlaylist[mCurrentVideo].toLocalFile();

  const QString wCutFilePath = mOutputRootDirectory + prettifyFileName(QFileInfo(wVideoPath).completeBaseName()) + "." + wStartTime.toString('.') + "." + QString::number((wEndTime - wStartTime).ms()) + ".mp4";

  mProcesses.push_back(std::make_unique<QProcess>(this));  
  connect(mProcesses.back().get(), &QProcess::started, this, [ & ]() {
    sequenceEntry.second = SequenceState::Processing;
    emit sequencesChanged(mSequenceMap);
    logStatusMessage("Precise cut started");
  });
  
  connect(mProcesses.back().get(), &QProcess::finished, this, [ & ](int exitCode, QProcess::ExitStatus exitStatus) {
    sequenceEntry.second = exitCode == 0 ? SequenceState::Succeeded : SequenceState::Failed;
    emit sequencesChanged(mSequenceMap);
    logStatusMessage(QString("Precise cut ") + (exitCode == 0 ? "succeeded" : "failed"));
  });
  
  const VTime wPreloadTime(1000);
  if (wStartTime >= wPreloadTime)
  {
    mProcesses.back()->start(mFFMpegPath, {
      "-ss", (wStartTime - wPreloadTime).toString(),
      "-i", wVideoPath,
      "-ss", wPreloadTime.toString(),
      "-t", (wEndTime - wStartTime).toString(),
      "-c:v", "libx264",
      "-c:a", "aac",
      wCutFilePath, "-y" });
  }
  else
  {
    mProcesses.back()->start(mFFMpegPath, {
      "-i", wVideoPath,
      "-ss", wStartTime.toString(),
      "-t", (wEndTime - wStartTime).toString(),
      "-c:v", "libx264",
      "-c:a", "aac",
      wCutFilePath, "-y" });
  }
}

void MediaPlayer::LoopCut(SequenceEntry& sequenceEntry, const quint32 loopCount)
{
  const VTime wStartTime = sequenceEntry.first.first;
  const VTime wEndTime = sequenceEntry.first.second;

  const QString wPrettyFileName = prettifyFileName(QFileInfo(mPlaylist[mCurrentVideo].toLocalFile()).completeBaseName());
  const QString wLengthStr = QString::number((wEndTime - wStartTime).ms());
  const QString wStartStr = wStartTime.toString('.');
  const QString wLoopFilePath = mOutputRootDirectory + wPrettyFileName + "." + wStartStr + "loop.mp4";

  // TODO: ugly nested process definitions down there. As these are dependent, we need to wait for the first one to finish before starting the second one
  // so the whole dependency scheduling is done in the previous process onfinished callback... in theory this is ok, but must have better implementation
  const QString wCutFilePath = QFileInfo(wLoopFilePath).absolutePath() + QFileInfo(wLoopFilePath).completeBaseName() + "_cut.mp4";

  mProcesses.push_back(std::make_unique<QProcess>(this));
  connect(mProcesses.back().get(), &QProcess::started, this, [&]() {
    sequenceEntry.second = SequenceState::Processing;
    emit sequencesChanged(mSequenceMap);
    logStatusMessage("Precise cut started");
  });

  connect(mProcesses.back().get(), &QProcess::finished, this, [&, wCutFilePath, wLoopFilePath, loopCount](int exitCode, QProcess::ExitStatus exitStatus) {
    logStatusMessage(QString("Precise cut ") + (exitCode == 0 ? "succeeded" : "failed"));
    if (exitCode != 0)
    {
      sequenceEntry.second = SequenceState::Failed;
      emit sequencesChanged(mSequenceMap);
      return;
    }

    const QString wReversedFilePath = QFileInfo(wCutFilePath).absolutePath() + QFileInfo(wCutFilePath).completeBaseName() + "_reversed.mp4";

    // Execute reverser process
    mProcesses.push_back(std::make_unique<QProcess>(this));
    connect(mProcesses.back().get(), &QProcess::started, this, [&]() {
      logStatusMessage("Reverser started");
    });

    connect(mProcesses.back().get(), &QProcess::finished, this, [&, wCutFilePath, wLoopFilePath, wReversedFilePath, loopCount](int exitCode, QProcess::ExitStatus exitStatus) {
      logStatusMessage(QString("Reverser ") + (exitCode == 0 ? "succeeded" : "failed"));
      if (exitCode != 0)
      {
        sequenceEntry.second = SequenceState::Failed;
        emit sequencesChanged(mSequenceMap);
        return;
      }

      // Merge the files
      // Create concat list file
      const QString wConcatFilePath = uniqueFileName(mOutputRootDirectory + "concat.txt");
      {
        std::ofstream ofs(wConcatFilePath.toStdString());
        for (unsigned n = 0; n < loopCount; ++n)
        {
          ofs << "file " << (wCutFilePath.toStdString()) << "\n";
          ofs << "file " << (wReversedFilePath.toStdString()) << "\n";
        }
      }

      // Execute merger process
      mProcesses.push_back(std::make_unique<QProcess>(this));
      connect(mProcesses.back().get(), &QProcess::started, this, [&]() {
        logStatusMessage("Merger started");
      });

      connect(mProcesses.back().get(), &QProcess::finished, this, [&, wConcatFilePath, wCutFilePath, wReversedFilePath](int exitCode, QProcess::ExitStatus exitStatus) {
        sequenceEntry.second = exitCode == 0 ? SequenceState::Succeeded : SequenceState::Failed;
        emit sequencesChanged(mSequenceMap);
        logStatusMessage(QString("Merger ") + (exitCode == 0 ? "succeeded" : "failed"));

        QFile::remove(wConcatFilePath);
        QFile::remove(wCutFilePath);
        QFile::remove(wReversedFilePath);
      });

      QStringList wArguments;
      wArguments << "-f" << "concat" << "-safe" << "0" << "-i" << wConcatFilePath << "-c" << "copy" << wLoopFilePath << "-y";
      mProcesses.back()->start(mFFMpegPath, wArguments);
    });

    mProcesses.back()->start(mFFMpegPath, {
      "-i", wCutFilePath,
      "-vf", "reverse",
      wReversedFilePath, "-y" });
  });

  // Exectute cut process
  const VTime wPreloadTime(1000);
  if (wStartTime >= wPreloadTime)
  {
    mProcesses.back()->start(mFFMpegPath, {
      "-ss", (wStartTime - wPreloadTime).toString(),
      "-i", mPlaylist[mCurrentVideo].toLocalFile(),
      "-ss", wPreloadTime.toString(),
      "-t", (wEndTime - wStartTime).toString(),
      "-c:v", "libx264",
      "-c:a", "aac",
      wCutFilePath, "-y" });
  }
  else
  {
    mProcesses.back()->start(mFFMpegPath, {
      "-i", mPlaylist[mCurrentVideo].toLocalFile(),
      "-ss", wStartTime.toString(),
      "-t", (wEndTime - wStartTime).toString(),
      "-c:v", "libx264",
      "-c:a", "aac",
      wCutFilePath, "-y" });
  }
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
