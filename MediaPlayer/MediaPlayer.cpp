#include "MediaPlayer.h"
#include "VideoPlayer.h"
#include "View.h"
#include "Utils.h"

#include <QFileInfo.h>
#include <QMediaMetadata.h>
#include <QUrl>
#include <QThread>
#include <QProcess>
#include <QMessageBox>
#include <QFileInfo>
#include <QDir>
#include <QRegularExpression>

#include <random>
#include <fstream>
#include <windows.h>
#include <filesystem>
#include <algorithm>

#undef min
#undef max

MediaPlayer::MediaPlayer(QObject* parent)
  : QObject(parent)
  , mView(std::make_shared<View>())
  , mPlayer(std::make_shared<VideoPlayer>(mView->getVideoWidget()))
{
  connect(mPlayer.get(), &VideoPlayer::positionChanged, this, [this](VTime position) { mView->setPosition(position); });
  connect(mPlayer.get(), &VideoPlayer::durationChanged, this, [this](VTime duration) { mView->setDuration(duration); });
  connect(mPlayer.get(), &VideoPlayer::videoLoaded,     this, &MediaPlayer::onVideoLoaded);
  connect(mPlayer.get(), &VideoPlayer::videoEnded,      this, &MediaPlayer::onVideoEnded);

  connect(mView.get(), &View::sliderChanged,          this, [this](int position) {setPosition(static_cast<VTime>(position), true); });
  connect(mView.get(), &View::previousButtonClicked,  this, [this]() { previous(); });
  connect(mView.get(), &View::startStopButtonClicked, this, [this]() { startStop(); });
  connect(mView.get(), &View::nextButtonClicked,      this, [this]() { next(); });
  connect(mView.get(), &View::audioButtonClicked,      this, [this]() { toggleAudio(); });
  connect(mView.get(), &View::seekLeftButtonClicked, this, [this]() { seek(SeekDirection::Backward, SeekStep::Random); });
  connect(mView.get(), &View::seekRightButtonClicked, this, [this]() { seek(SeekDirection::Forward, SeekStep::Random); });
  connect(mView.get(), &View::deinterlaceChecked, this, [this](const bool state) { setDeinterlace(state); });
  connect(mView.get(), &View::gpuEncodeChecked, this, [this](const bool state) { setGpuEncode(state); });  
  
  connect(mView.get(), &View::sequenceSelected, this, [ this ](const Sequence* wSequence) { 
    wSequence == nullptr ? 
      mView->setDurationLabel(mPlayer->getDuration()) 
      : mView->setDurationLabel(wSequence->second - wSequence->first, true); // TODO solve the coloring of the label in a better way
    mSelectedSequence = wSequence; });
  connect(mView.get(), &View::sequenceDoubleClicked, this, [ this ](const Sequence* wSequence) 
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

  connect(this, &MediaPlayer::sequencesChanged, mView.get(), &View::onSequencesChanged);

  // if a +filter button is clicked on the view, ask for a filter pattern and if ok, view will emit an event with the pattern
  // controller catches this event and creates a new filter then emits a Filter changed event
  // for the filter changed event the view updates it`s own filter UI element list with the given filter list of the event

  mPlayer->setVolume(50);
  //mPlayer->setPlaybackRate(0.5f);
}

MediaPlayer::~MediaPlayer()
{}

void MediaPlayer::setPlaylist(const Playlist& playlist)
{
  if (playlist.empty())
  {
    logStatusMessage("Playlist is empty.");
    return;
  }

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
    setPosition(VTime(0), !isPlaying);
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
    setPosition(VTime(0), !isPlaying);
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

void MediaPlayer::setDeinterlace(const bool state)
{
  mDeinterlace = state;
}

void MediaPlayer::setGpuEncode(const bool state)
{
  mGpuEncode = state;
}

void MediaPlayer::setPosition(const VTime& position, const bool updateNeeded)
{
  mPlayer->setPosition(position, updateNeeded);
}

VTime MediaPlayer::getPosition() const
{
  return mPlayer->getPosition();
}

void MediaPlayer::seek(MediaPlayer::SeekDirection direction, MediaPlayer::SeekStep step)
{
  VTime wStepSize;
  switch (step)
  {
    case SeekStep::Small:
      wStepSize = VTime("00:00:00.025");
      break;
    case SeekStep::Big:
      wStepSize = VTime("00:00:05.000");
      break;
    case SeekStep::Normal:
      wStepSize = VTime("00:00:00.500");
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

      qint64 wMin = 10;
      qint64 wMax = std::max(wMin, wSeekWindow.ms());
      wStepSize = VTime(Random(wMin, wMax));  // assuming 10 ms is left from the video, at least..
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
}

void MediaPlayer::onVideoEnded()
{
  // TODO add loop|next|stop Settings
  next();
}

void MediaPlayer::FastCut(SequenceEntry& sequenceEntry)
{
  const VTime wStartTime = sequenceEntry.first.first;
  const VTime wEndTime = sequenceEntry.first.second;
  const QString& wVideoPath = mPlaylist[mCurrentVideo].toLocalFile();

  const QString wCutFilePath = mOutputRootDirectory + prettifyFileName(QFileInfo(wVideoPath).completeBaseName()) + "." + wStartTime.toString('.') + "." + QString::number((wEndTime - wStartTime).ms()) + ".mp4";
  sequenceEntry.second.mFilePath = wCutFilePath;

  mProcesses.push_back(std::make_unique<QProcess>(this));
  QProcess* wProcess = mProcesses.back().get();

  connect(wProcess, &QProcess::started, this, [ & ]() {
    sequenceEntry.second.mState = OperationState::Processing;
    emit sequencesChanged(mSequenceMap);
    logStatusMessage("Fast cut started");
  });

  connect(wProcess, &QProcess::finished, this, [ & ](int exitCode, QProcess::ExitStatus exitStatus) {
    sequenceEntry.second.mState = exitCode == 0 ? OperationState::Succeeded : OperationState::Failed;
    emit sequencesChanged(mSequenceMap);
    logStatusMessage(QString("Fast cut ") + (exitCode == 0 ? "succeeded" : "failed"));
  });

  connect(wProcess, &QProcess::readyReadStandardOutput, this, [ &, this, wProcess]() {
    QString output = QString::fromLocal8Bit(wProcess->readAllStandardOutput());
    if (!output.isEmpty())
    {
      QRegularExpression re(R"(time.*?(\d{2}:\d{2}:\d{2}\.\d{2}))");
      QRegularExpressionMatchIterator i = re.globalMatch(output);
      while (i.hasNext()) {
        QRegularExpressionMatch match = i.next();
        if (match.hasMatch()) {
          const QString timeStr = match.captured(1); // "hh:mm:ss.mm"
          const VTime time = VTime(timeStr);
          const VTime duration = sequenceEntry.first.second - sequenceEntry.first.first;
          if (time < duration)
          {
            sequenceEntry.second.mProcessTimer = time;
          }
          else
          {
            sequenceEntry.second.mProcessTimer = duration;
          }
          emit sequencesChanged(mSequenceMap);
        }
      }
    }
    });
  connect(wProcess, &QProcess::readyReadStandardError, this, [ &, this, wProcess]() {
    QString error = QString::fromLocal8Bit(wProcess->readAllStandardError());
    if (!error.isEmpty())
    {
        QRegularExpression re(R"(time.*?(\d{2}:\d{2}:\d{2}\.\d{2}))");
        QRegularExpressionMatchIterator i = re.globalMatch(error);
        while (i.hasNext()) {
          QRegularExpressionMatch match = i.next();
          if (match.hasMatch()) {
            const QString timeStr = match.captured(1); // "hh:mm:ss.mm"
            const VTime time = VTime(timeStr);
            const VTime duration = sequenceEntry.first.second - sequenceEntry.first.first;
            if (time < duration)
            {
              sequenceEntry.second.mProcessTimer = time;
            }
            else
            {
              sequenceEntry.second.mProcessTimer = duration;
            }
            emit sequencesChanged(mSequenceMap);
          }
      }
    }
    });

  QStringList args;
  {
    args = { "-ss", wStartTime.toString(),
             "-i", wVideoPath,
             "-t", (wEndTime - wStartTime).toString(),
             "-async", "1",
             "-vcodec", "copy",
             "-acodec", "copy",
             "-avoid_negative_ts", "1",
             wCutFilePath, "-y" };
  }
  wProcess->start(mFFMpegPath, args);
}

void MediaPlayer::PreciseCut(SequenceEntry& sequenceEntry)
{
  const VTime wStartTime = sequenceEntry.first.first;
  const VTime wEndTime = sequenceEntry.first.second;
  const QString& wVideoPath = mPlaylist[mCurrentVideo].toLocalFile();

  const QString wCutFilePath = mOutputRootDirectory + prettifyFileName(QFileInfo(wVideoPath).completeBaseName()) + "." + wStartTime.toString('.') + "." + QString::number((wEndTime - wStartTime).ms()) + ".mp4";
  sequenceEntry.second.mFilePath = wCutFilePath;

  QStringList args;
  {
    args = { "-hide_banner", "-loglevel", "info", "-y" };
    if (mGpuEncode)
    {
      args.append({ "-hwaccel", "cuda" });
    }

    const VTime wPreloadTime(1000);
    if (wStartTime >= wPreloadTime)
    {
      args.append({
          "-ss", (wStartTime - wPreloadTime).toString(),
          "-i", wVideoPath,
          "-ss", wPreloadTime.toString(),
        });
    }
    else
    {
      args.append({
          "-i", wVideoPath,
          "-ss", wStartTime.toString(),
          "-t", (wEndTime - wStartTime).toString(),
        });
    }
    args.append({ "-t", (wEndTime - wStartTime).toString() });
    if (mDeinterlace)
    {
      args.append({ "-vf", "yadif" });
    }

    args.append({ "-c:v", mGpuEncode ? "h264_nvenc" : "libx264" }); // GPU or CPU
    args.append({ "-c:a", "copy" }); //"aac" if needed
    args.append(wCutFilePath);
  }

  mProcesses.push_back(std::make_unique<QProcess>(this));
  QProcess* wProcess = mProcesses.back().get();
  connect(wProcess, &QProcess::started, this, [&]() {
    sequenceEntry.second.mState = OperationState::Processing;
    emit sequencesChanged(mSequenceMap);
    logStatusMessage(QString("Precise cut started on ") + (mGpuEncode ? "GPU" : "CPU") + (mDeinterlace ? " with deinterlacing" : ""));
    });

  connect(wProcess, &QProcess::finished, this, [&](int exitCode, QProcess::ExitStatus exitStatus) {
    sequenceEntry.second.mState = exitCode == 0 ? OperationState::Succeeded : OperationState::Failed;
    emit sequencesChanged(mSequenceMap);
    logStatusMessage(QString("Precise cut ") + (exitCode == 0 ? "succeeded" : "failed"));
    });

  connect(wProcess, &QProcess::readyReadStandardOutput, this, [&, this, wProcess]() {
    QString output = QString::fromLocal8Bit(wProcess->readAllStandardOutput());
    if (!output.isEmpty())
    {
      QRegularExpression re(R"(time.*?(\d{2}:\d{2}:\d{2}\.\d{2}))");
      QRegularExpressionMatchIterator i = re.globalMatch(output);
      while (i.hasNext()) {
        QRegularExpressionMatch match = i.next();
        if (match.hasMatch()) {
          const QString timeStr = match.captured(1); // "hh:mm:ss.mm"
          const VTime time = VTime(timeStr);
          const VTime duration = sequenceEntry.first.second - sequenceEntry.first.first;
          if (time < duration)
          {
            sequenceEntry.second.mProcessTimer = time;
          }
          else
          {
            sequenceEntry.second.mProcessTimer = duration;
          }
          emit sequencesChanged(mSequenceMap);
        }
      }
    }
    });
  connect(wProcess, &QProcess::readyReadStandardError, this, [&, this, wProcess]() {
    QString error = QString::fromLocal8Bit(wProcess->readAllStandardError());
    if (!error.isEmpty())
    {
      QRegularExpression re(R"(time.*?(\d{2}:\d{2}:\d{2}\.\d{2}))");
      QRegularExpressionMatchIterator i = re.globalMatch(error);
      while (i.hasNext()) {
        QRegularExpressionMatch match = i.next();
        if (match.hasMatch()) {
          const QString timeStr = match.captured(1); // "hh:mm:ss.mm"
          const VTime time = VTime(timeStr);
          const VTime duration = sequenceEntry.first.second - sequenceEntry.first.first;
          if (time < duration)
          {
            sequenceEntry.second.mProcessTimer = time;
          }
          else
          {
            sequenceEntry.second.mProcessTimer = duration;
          }
          emit sequencesChanged(mSequenceMap);
        }
      }
    }
    });

  wProcess->start(mFFMpegPath, args);
}

void MediaPlayer::LoopCut(SequenceEntry& sequenceEntry)
{
  const VTime wStartTime = sequenceEntry.first.first;
  const VTime wEndTime = sequenceEntry.first.second;

  const QString wPrettyFileName = prettifyFileName(QFileInfo(mPlaylist[mCurrentVideo].toLocalFile()).completeBaseName());
  const QString wLengthStr = QString::number((wEndTime - wStartTime).ms());
  const QString wStartStr = wStartTime.toString('.');
  const QString wLoopFilePath = mOutputRootDirectory + wPrettyFileName + "." + wStartStr + ".loop.mp4";
  const unsigned loopCount = mView->getLoopCount();
  sequenceEntry.second.mFilePath = wLoopFilePath;

  // TODO: ugly nested process definitions down there. As these are dependent, we need to wait for the first one to finish before starting the second one
  // so the whole dependency scheduling is done in the previous process onfinished callback... in theory this is ok, but must have better implementation
  const QString wCutFilePath = QFileInfo(wLoopFilePath).absolutePath() + QFileInfo(wLoopFilePath).completeBaseName() + "_cut.mp4";

  mProcesses.push_back(std::make_unique<QProcess>(this));
  QProcess* wCutProcess = mProcesses.back().get();
  connect(wCutProcess, &QProcess::started, this, [&]() {
    sequenceEntry.second.mState = OperationState::Processing;
    sequenceEntry.second.mProcessTimer = VTime(0);
    emit sequencesChanged(mSequenceMap);
    logStatusMessage(QString("Loop cut started on ") + (mGpuEncode ? "GPU" : "CPU") + (mDeinterlace ? " with deinterlacing" : ""));
    });

  connect(wCutProcess, &QProcess::finished, this, [&, wCutFilePath, wLoopFilePath, loopCount](int exitCode, QProcess::ExitStatus exitStatus) {
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
    QProcess* wReverserProcess = mProcesses.back().get();
    connect(wReverserProcess, &QProcess::started, this, [&]() {
      sequenceEntry.second.mProcessTimer = VTime(0);
      emit sequencesChanged(mSequenceMap);
      logStatusMessage("Reverser started");
      });

    connect(wReverserProcess, &QProcess::finished, this, [&, wCutFilePath, wLoopFilePath, wReversedFilePath, loopCount](int exitCode, QProcess::ExitStatus exitStatus) {
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
      QProcess* wMergerProcess = mProcesses.back().get();
      connect(wMergerProcess, &QProcess::started, this, [&]() {
        sequenceEntry.second.mProcessTimer = VTime(0);
        emit sequencesChanged(mSequenceMap);
        logStatusMessage("Merger started");
        });

      connect(wMergerProcess, &QProcess::finished, this, [&, wConcatFilePath, wCutFilePath, wReversedFilePath](int exitCode, QProcess::ExitStatus exitStatus) {
        sequenceEntry.second.mState = exitCode == 0 ? OperationState::Succeeded : OperationState::Failed;
        emit sequencesChanged(mSequenceMap);
        logStatusMessage(QString("Loop cut ") + (exitCode == 0 ? "succeeded" : "failed"));

        QFile::remove(wConcatFilePath);
        QFile::remove(wCutFilePath);
        QFile::remove(wReversedFilePath);
        });

      connect(wMergerProcess, &QProcess::readyReadStandardOutput, this, [&, this, wMergerProcess]() {
        QString output = QString::fromLocal8Bit(wMergerProcess->readAllStandardOutput());
        if (!output.isEmpty())
        {
          QRegularExpression re(R"(time.*?(\d{2}:\d{2}:\d{2}\.\d{2}))");
          QRegularExpressionMatchIterator i = re.globalMatch(output);
          while (i.hasNext()) {
            QRegularExpressionMatch match = i.next();
            if (match.hasMatch()) {
              const QString timeStr = match.captured(1); // "hh:mm:ss.mm"
              const VTime time = VTime(timeStr);
              const VTime duration = sequenceEntry.first.second - sequenceEntry.first.first;
              if (time < duration)
              {
                sequenceEntry.second.mProcessTimer = time;
              }              
              else
              {
                sequenceEntry.second.mProcessTimer = duration;
              }
              emit sequencesChanged(mSequenceMap);
            }
          }
        }
        });
      connect(wMergerProcess, &QProcess::readyReadStandardError, this, [&, this, wMergerProcess]() {
        QString error = QString::fromLocal8Bit(wMergerProcess->readAllStandardError());
        if (!error.isEmpty())
        {
          QRegularExpression re(R"(time.*?(\d{2}:\d{2}:\d{2}\.\d{2}))");
          QRegularExpressionMatchIterator i = re.globalMatch(error);
          while (i.hasNext()) {
            QRegularExpressionMatch match = i.next();
            if (match.hasMatch()) {
              const QString timeStr = match.captured(1); // "hh:mm:ss.mm"
              const VTime time = VTime(timeStr);
              const VTime duration = sequenceEntry.first.second - sequenceEntry.first.first;
              if (time < duration)
              {
                sequenceEntry.second.mProcessTimer = time;
              }
              else
              {
                sequenceEntry.second.mProcessTimer = duration;
              }
              emit sequencesChanged(mSequenceMap);
            }
          }
        }
        });

      QStringList wArguments;
      wArguments << "-f" << "concat" << "-safe" << "0" << "-i" << wConcatFilePath << "-c" << "copy" << wLoopFilePath << "-y";
      wMergerProcess->start(mFFMpegPath, wArguments);
      });

    connect(wReverserProcess, &QProcess::readyReadStandardOutput, this, [&, this, wReverserProcess]() {
      QString output = QString::fromLocal8Bit(wReverserProcess->readAllStandardOutput());
      if (!output.isEmpty())
      {
        QRegularExpression re(R"(time.*?(\d{2}:\d{2}:\d{2}\.\d{2}))");
        QRegularExpressionMatchIterator i = re.globalMatch(output);
        while (i.hasNext()) {
          QRegularExpressionMatch match = i.next();
          if (match.hasMatch()) {
            const QString timeStr = match.captured(1); // "hh:mm:ss.mm"
            const VTime time = VTime(timeStr);
            const VTime duration = sequenceEntry.first.second - sequenceEntry.first.first;
            if (time < duration)
            {
              sequenceEntry.second.mProcessTimer = time;
            }
            else
            {
              sequenceEntry.second.mProcessTimer = duration;
            }
            emit sequencesChanged(mSequenceMap);
          }
        }
      }
      });

    connect(wReverserProcess, &QProcess::readyReadStandardError, this, [&, this, wReverserProcess]() {
      QString error = QString::fromLocal8Bit(wReverserProcess->readAllStandardError());
      if (!error.isEmpty())
      {
        QRegularExpression re(R"(time.*?(\d{2}:\d{2}:\d{2}\.\d{2}))");
        QRegularExpressionMatchIterator i = re.globalMatch(error);
        while (i.hasNext()) {
          QRegularExpressionMatch match = i.next();
          if (match.hasMatch()) {
            const QString timeStr = match.captured(1); // "hh:mm:ss.mm"
            const VTime time = VTime(timeStr);
            const VTime duration = sequenceEntry.first.second - sequenceEntry.first.first;
            if (time < duration)
            {
              sequenceEntry.second.mProcessTimer = time;
            }
            else
            {
              sequenceEntry.second.mProcessTimer = duration;
            }
            emit sequencesChanged(mSequenceMap);
          }
        }
      }
      });

    wReverserProcess->start(mFFMpegPath, {
      "-i", wCutFilePath,
      "-vf", "reverse",
      "-af", "areverse",
      wReversedFilePath, "-y" });
    });

  connect(wCutProcess, &QProcess::readyReadStandardOutput, this, [&, this, wCutProcess]() {
    QString output = QString::fromLocal8Bit(wCutProcess->readAllStandardOutput());
    if (!output.isEmpty())
    {
      QRegularExpression re(R"(time.*?(\d{2}:\d{2}:\d{2}\.\d{2}))");
      QRegularExpressionMatchIterator i = re.globalMatch(output);
      while (i.hasNext()) {
        QRegularExpressionMatch match = i.next();
        if (match.hasMatch()) {
          const QString timeStr = match.captured(1); // "hh:mm:ss.mm"
          const VTime time = VTime(timeStr);
          const VTime duration = sequenceEntry.first.second - sequenceEntry.first.first;
          if (time < duration)
          {
            sequenceEntry.second.mProcessTimer = time;
          }
          else
          {
            sequenceEntry.second.mProcessTimer = duration;
          }
          emit sequencesChanged(mSequenceMap);
        }
      }
    }
    });
  connect(wCutProcess, &QProcess::readyReadStandardError, this, [&, this, wCutProcess]() {
    QString error = QString::fromLocal8Bit(wCutProcess->readAllStandardError());
    if (!error.isEmpty())
    {
      QRegularExpression re(R"(time.*?(\d{2}:\d{2}:\d{2}\.\d{2}))");
      QRegularExpressionMatchIterator i = re.globalMatch(error);
      while (i.hasNext()) {
        QRegularExpressionMatch match = i.next();
        if (match.hasMatch()) {
          const QString timeStr = match.captured(1); // "hh:mm:ss.mm"
          const VTime time = VTime(timeStr);
          const VTime duration = sequenceEntry.first.second - sequenceEntry.first.first;
          if (time < duration)
          {
            sequenceEntry.second.mProcessTimer = time;
          }
          else
          {
            sequenceEntry.second.mProcessTimer = duration;
          }
          emit sequencesChanged(mSequenceMap);
        }
      }
    }
    });

  // Exectute cut process
  QStringList args;
  {
    args = { "-hide_banner", "-loglevel", "info", "-y" };
    if (mGpuEncode)
    {
      args.append({ "-hwaccel", "cuda" });
    }

    const VTime wPreloadTime(1000);
    if (wStartTime >= wPreloadTime)
    {
      args = {
          "-ss", (wStartTime - wPreloadTime).toString(),
          "-i", mPlaylist[mCurrentVideo].toLocalFile(),
          "-ss", wPreloadTime.toString(),
      };
    }
    else
    {
      args = {
          "-i", mPlaylist[mCurrentVideo].toLocalFile(),
          "-ss", wStartTime.toString(),
          "-t", (wEndTime - wStartTime).toString(),
      };
    }

    args.append({ "-t", (wEndTime - wStartTime).toString() });

    if (mDeinterlace)
    {
      args.append({ "-vf", "yadif" });
    }
    args.append({ "-c:v", mGpuEncode ? "h264_nvenc" : "libx264" }); // GPU or CPU
    args.append({ "-c:a", "aac",
                  wCutFilePath });
  }

  wCutProcess->start(mFFMpegPath, args);
}

void MediaPlayer::resetSeqenceState()
{
  for (auto& wSequence : mSequenceMap)
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

void MediaPlayer::deleteCurrentVideo()
{
  //if (mPlaylist.empty() || mCurrentVideo >= mPlaylist.size())
  {
    return;
  }

  const QString filePath = mPlaylist[mCurrentVideo].toLocalFile();

  const QFileInfo fileInfo(filePath);
  const QString dirPath = QDir::cleanPath(fileInfo.absolutePath());

  // Check if the directory is in the allowed folders
  bool allowed = std::any_of(
    mSettings.mRawFolders.begin(), mSettings.mRawFolders.end(),
    [&dirPath](const std::string& allowedFolder)
    {
      return dirPath.compare(QDir::cleanPath(QString::fromStdString(allowedFolder)), Qt::CaseInsensitive) == 0;
    }
  );

  // if readonly do not delete
  if (!fileInfo.permissions().testFlag(QFileDevice::WriteUser))
  {
    logStatusMessage(QString("Deletion not allowed: %1 is read-only.").arg(filePath));
    return;
  }

  if (!allowed)
  {
    logStatusMessage(QStringLiteral("Deletion not allowed: %1 is not in allowed folders.").arg(dirPath));
    return;
  }

  QMessageBox::StandardButton reply = QMessageBox::warning(
    nullptr,
    QStringLiteral("Delete Video"),
    QStringLiteral("Are you sure you want to delete?\n\n%1").arg(filePath),
    QMessageBox::Ok | QMessageBox::Cancel
  );

  if (reply == QMessageBox::Ok)
  {
    //got to next or unload video if single file and try deleting source file.
    next();

    //std::error_code ec;
    //try { std::filesystem::remove(filePath.toStdWString(), ec); } catch (...) { }
    //
    //if (!ec)
    //{
    //  logStatusMessage(QString("File deleted: %1").arg(filePath));
    //  
    //TODO: refactor - same as next/previous 
    //  mPlaylist.erase(mPlaylist.begin() + mCurrentVideo);
    //  mCurrentVideo = std::min(mCurrentVideo, mPlaylist.size() - 1);
    //  
    //  mPlayer->setVideo(mPlaylist[mCurrentVideo]);
    //
    //  mSequenceMap.clear();
    //  mSelectedSequence = nullptr;
    //  emit sequencesChanged(mSequenceMap);  // TODO: store the sequences associated to the video, ...
    //}
    //else
    //{
    //  logStatusMessage(QString("Error deleting file %1: %2").arg(filePath).arg(QString::fromStdString(ec.message())));
    //}
  }
}

void MediaPlayer::addFilter(const MediaPlayer::Filter& filter)
{
  mFilters.push_back(filter);
  emit filtersChanged(mFilters);
}

void MediaPlayer::burstCut()
{
  const VTime wBacktrackTime(mView->getBurstLength() * 0.1); // 10 % backtrack time

  for (unsigned n = 0; n < mView->getLoopCount(); ++n)
  {
    mark();
    setPosition(getPosition() + mView->getBurstLength(), false);
    mark();
    setPosition(getPosition() - wBacktrackTime, true);
  }
  cut(CutMethod::Precise);
}