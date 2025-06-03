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

  QObject::connect(mView.get(), &View::sliderChanged,          this, [this](int position) {setPosition(static_cast<VTime>(position)); });
  QObject::connect(mView.get(), &View::previousButtonClicked,  this, [this]() { previous(); });
  QObject::connect(mView.get(), &View::startStopButtonClicked, this, [this]() { startStop(); });
  QObject::connect(mView.get(), &View::nextButtonClicked,      this, [this]() { next(); });
  QObject::connect(mView.get(), &View::audioButtonClicked,      this, [this]() { toggleAudio(); });
  
  QObject::connect(mView.get(), &View::sequenceSelected, this, [ this ](const Sequence* wSequence) { 
    wSequence == nullptr ? 
      mView->setDurationLabel(mPlayer->getDuration()) 
      : mView->setDurationLabel(wSequence->second - wSequence->first, true); // TODO solve the coloring of the label in a better way
    mSelectedSequence = wSequence; });
  QObject::connect(mView.get(), &View::sequenceDoubleClicked, this, [ this ](const Sequence* wSequence) 
  {
    qDebug() << "double clicked sequence" << wSequence->first.ms() << wSequence->second.ms();
    QString wFileName = QFileInfo(mPlaylist[mCurrentVideo].toString()).fileName();

    auto wSequenceEntry = mSequenceMap.find(*wSequence);
    if (wSequenceEntry == mSequenceMap.end())
    {
      return;
    }
  
    if (wSequenceEntry->second.mState != OperationState::Succeeded || !QFile(wSequenceEntry->second.mFilePath).exists())
    {
      return;
    }

    qDebug() << "executing" << wSequenceEntry->second.mFilePath;
    QProcess::startDetached("explorer.exe", { wSequenceEntry->second.mFilePath });
  });

  QObject::connect(this, &MediaPlayer::sequencesChanged, mView.get(), &View::onSequencesChanged);

  mPlayer->setVolume(50);
  //mPlayer->setPlaybackRate(0.5f);
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
  
  switch (mSettings.mAudioMode)
  {
    case Settings::AudioMode::Muted:
    case Settings::AudioMode::Music:
      mPlayer->setMuted(true);
      break;
    case Settings::AudioMode::Video:
      mPlayer->setMuted(false);
      break;
  }

  mView->toggleAudio(mSettings.mAudioMode);
  mView->setCursorTimeout(mSettings.mCursorTimeout);
}

const Settings& MediaPlayer::getSettings() const
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
    setPosition(VTime(0));
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
    mSelectedSequence = nullptr;
    emit sequencesChanged(mSequenceMap);  // TODO: store the sequences associated to the video, ...
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
    setPosition(VTime(0));
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

void MediaPlayer::toggleAudio()
{
  auto modeInt = static_cast<std::underlying_type<Settings::AudioMode>::type>(mSettings.mAudioMode);
  modeInt = (modeInt + 1) % static_cast<int>(Settings::AudioMode::Last);
  mSettings.mAudioMode = static_cast<Settings::AudioMode>(modeInt);

  switch (mSettings.mAudioMode)
  {
    case Settings::AudioMode::Muted:
    case Settings::AudioMode::Music:
      mPlayer->setMuted(true);
      break;
    case Settings::AudioMode::Video:
      mPlayer->setMuted(false);
      break;
  }

  mView->toggleAudio(mSettings.mAudioMode);
}

void MediaPlayer::setPosition(const VTime& position)
{
  mPlayer->setPosition(position);
}

void MediaPlayer::seek(MediaPlayer::SeekDirection direction, MediaPlayer::SeekStep step)
{
  VTime wStepSize;
  switch (step)
  {
    case SeekStep::Small:
      wStepSize = VTime(25);  // TODO use settings
      break;
    case SeekStep::Big:
      wStepSize = VTime(5000);
      break;
    case SeekStep::Normal:
      wStepSize = VTime(500);
      break;
    case SeekStep::Random:
    {
      const VTime wDuration = mPlayer->getDuration();
      const VTime wPosition = mPlayer->getPosition();
      const VTime wRemaining = mPlayer->getDuration() - mPlayer->getPosition();

      VTime wSeekWindow = VTime(static_cast<qint64>(wDuration.ms() / 5.0f));
      if (direction == SeekDirection::Forward)
      {
        if (wPosition + wSeekWindow >= wDuration)
        {
          wSeekWindow = wRemaining;
        }
      }
      else if(direction == SeekDirection::Backward)
      {
        if (wPosition - wSeekWindow < VTime(0))
        {
          wSeekWindow = wPosition;
        }
      }

      wStepSize = VTime(Random(qint64(10), wSeekWindow.ms()));  // assuming 10 ms is left from the video, at leat..
    }
  }

  if (direction == SeekDirection::Forward)
  {
    mPlayer->seekForward(wStepSize);
  }
  else if (direction == SeekDirection::Backward)
  {
    mPlayer->seekBackward(wStepSize);
  }
  else
  {
    throw std::runtime_error("Invalid seek direction");
  }
}

void MediaPlayer::snapToSelection(MediaPlayer::SnapPosition position)
{
  if (mSelectedSequence == nullptr)
  {
    return;
  }

  switch (position)
  {
    case SnapPosition::Start:
    mPlayer->setPosition(mSelectedSequence->first);
    break;
    case SnapPosition::End:
    mPlayer->setPosition(mSelectedSequence->second);
    break;
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

void MediaPlayer::mark(const bool isCancel)
{
  if (isCancel)
  {
    mEditedSequence = Sequence{ VTime(0), VTime(0) };
    mView->setMarking(false);
    return;
  }

  if (mEditedSequence == Sequence{ VTime(0), VTime(0) })
  {
    mEditedSequence.first = mPlayer->getPosition();
    mEditedSequence.second = VTime(1); // dummy value, just to allow sequences start with 0

    mView->setMarking(true);
  }
  else
  {
    mView->setMarking(false);

    mEditedSequence.second = mPlayer->getPosition();

    if (mEditedSequence.second - mEditedSequence.first < VTime(1))
    {
      mEditedSequence = Sequence{ VTime(0), VTime(0) };
      return;
    }

    mSequenceMap[mEditedSequence].mState = OperationState::Ready;

    mEditedSequence = Sequence{ VTime(0), VTime(0) };
    emit sequencesChanged(mSequenceMap);
  }
}

void MediaPlayer::logStatusMessage(const QString& msg)
{
  mView->setInfo(msg);
}

void MediaPlayer::cut(const CutMethod cutMethod)
{
  if (mSequenceMap.empty())
  {
    return;
  }

  if (mSelectedSequence != nullptr)
  {
    auto wSequenceEntryIt = mSequenceMap.find(*mSelectedSequence);
    if (wSequenceEntryIt == mSequenceMap.end() || wSequenceEntryIt->second.mState != OperationState::Ready)
    {
      return;
    }

    switch (cutMethod)
    {
      case CutMethod::Fast:
        FastCut(*wSequenceEntryIt);
        break;
      case CutMethod::Precise:
        PreciseCut(*wSequenceEntryIt);
        break;
      case CutMethod::Loop:
        LoopCut(*wSequenceEntryIt);
        break;
    }
  }
  else
  {
    for (auto& wSequenceEntry : mSequenceMap)
    {
      if (wSequenceEntry.second.mState != OperationState::Ready)
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
          LoopCut(wSequenceEntry);
        break;
      }
    }
  }
}

void MediaPlayer::onVideoLoaded()
{ 
  const QFileInfo fileInfo(mPlaylist[mCurrentVideo].toLocalFile());
  const QString info(fileInfo.completeBaseName() + " - " + QString::number(mPlayer->getMetadata().value(QMediaMetaData::Resolution).value<QSize>().width()) + " x " + QString::number(mPlayer->getMetadata().value(QMediaMetaData::Resolution).value<QSize>().height()));
  mView->setInfo(info);

  setPosition(VTime(0));

  if (mSettings.mAutoPlay)
  {
    play();
  }
  else if (mSettings.mShowFirstFrame)
  {
    pause();
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
  else if (mSettings.mShowFirstFrame)
  {
    pause();
  }
}

void MediaPlayer::FastCut(SequenceEntry& sequenceEntry)
{
  const VTime wStartTime = sequenceEntry.first.first;
  const VTime wEndTime = sequenceEntry.first.second;
  const QString& wVideoPath = mPlaylist[mCurrentVideo].toLocalFile();

  const QString wCutFilePath = mOutputRootDirectory + prettifyFileName(QFileInfo(wVideoPath).completeBaseName()) + "." + wStartTime.toString('.') + "." + QString::number((wEndTime - wStartTime).ms()) + ".mp4";
  sequenceEntry.second.mFilePath = wCutFilePath;

  mProcesses.push_back(std::make_unique<QProcess>(this));
  connect(mProcesses.back().get(), &QProcess::started, this, [ & ]() {
    sequenceEntry.second.mState = OperationState::Processing;
    emit sequencesChanged(mSequenceMap);
    logStatusMessage("Fast cut started");
  });

  connect(mProcesses.back().get(), &QProcess::finished, this, [ & ](int exitCode, QProcess::ExitStatus exitStatus) {
    sequenceEntry.second.mState = exitCode == 0 ? OperationState::Succeeded : OperationState::Failed;
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
  sequenceEntry.second.mFilePath = wCutFilePath;

  mProcesses.push_back(std::make_unique<QProcess>(this));  
  connect(mProcesses.back().get(), &QProcess::started, this, [ & ]() {
    sequenceEntry.second.mState = OperationState::Processing;
    emit sequencesChanged(mSequenceMap);
    logStatusMessage("Precise cut started");
  });
  
  connect(mProcesses.back().get(), &QProcess::finished, this, [ & ](int exitCode, QProcess::ExitStatus exitStatus) {
    sequenceEntry.second.mState = exitCode == 0 ? OperationState::Succeeded : OperationState::Failed;
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

void MediaPlayer::LoopCut(SequenceEntry& sequenceEntry)
{
  const VTime wStartTime = sequenceEntry.first.first;
  const VTime wEndTime = sequenceEntry.first.second;

  const QString wPrettyFileName = prettifyFileName(QFileInfo(mPlaylist[mCurrentVideo].toLocalFile()).completeBaseName());
  const QString wLengthStr = QString::number((wEndTime - wStartTime).ms());
  const QString wStartStr = wStartTime.toString('.');
  const QString wLoopFilePath = mOutputRootDirectory + wPrettyFileName + "." + wStartStr + "loop.mp4";
  const unsigned loopCount = mView->getLoopCount();
  sequenceEntry.second.mFilePath = wLoopFilePath;

  // TODO: ugly nested process definitions down there. As these are dependent, we need to wait for the first one to finish before starting the second one
  // so the whole dependency scheduling is done in the previous process onfinished callback... in theory this is ok, but must have better implementation
  const QString wCutFilePath = QFileInfo(wLoopFilePath).absolutePath() + QFileInfo(wLoopFilePath).completeBaseName() + "_cut.mp4";

  mProcesses.push_back(std::make_unique<QProcess>(this));
  connect(mProcesses.back().get(), &QProcess::started, this, [&]() {
    sequenceEntry.second.mState = OperationState::Processing;
    emit sequencesChanged(mSequenceMap);
    logStatusMessage("Loop cut started");
  });

  connect(mProcesses.back().get(), &QProcess::finished, this, [&, wCutFilePath, wLoopFilePath, loopCount](int exitCode, QProcess::ExitStatus exitStatus) {
    logStatusMessage(QString("Precise cut ") + (exitCode == 0 ? "succeeded" : "failed"));
    if (exitCode != 0)
    {
      sequenceEntry.second.mState = OperationState::Failed;
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
        sequenceEntry.second.mState = OperationState::Failed;
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
        sequenceEntry.second.mState = exitCode == 0 ? OperationState::Succeeded : OperationState::Failed;
        emit sequencesChanged(mSequenceMap);
        logStatusMessage(QString("Loop cut ") + (exitCode == 0 ? "succeeded" : "failed"));

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
      "-af", "areverse",
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
    wSequence.second.mState = OperationState::Ready;
  }
  emit sequencesChanged(mSequenceMap);
}

void MediaPlayer::deleteSequence()
{
  if (mSequenceMap.empty() || mSelectedSequence == nullptr)
  {
    return;
  }

  auto wSequenceEntryIt = mSequenceMap.find(*mSelectedSequence);
  if (wSequenceEntryIt == mSequenceMap.end())
  {
    return;
  }

  if (QFile::exists(wSequenceEntryIt->second.mFilePath))
  {
    try
    {
      QFile::remove(wSequenceEntryIt->second.mFilePath);
      logStatusMessage(QString("%1 successfully deleted").arg(wSequenceEntryIt->second.mFilePath));
    }
    catch (const std::exception& e)
    {
      logStatusMessage(QString("Deleting file %1: failed! Error: %2").arg(wSequenceEntryIt->second.mFilePath).arg(e.what()));
    }
  }

  mSequenceMap.erase(*mSelectedSequence);
  emit sequencesChanged(mSequenceMap);
}
