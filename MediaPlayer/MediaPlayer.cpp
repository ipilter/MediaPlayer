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
#include <filesystem>
#include <algorithm>

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
  connect(mView.get(), &View::randomizeChanged, this, [this](const bool state) 
    {
      mPlaylist.setOrder(state);
      mSettings.mRandomize = state;
    });
  connect(mView.get(), &View::onMouseClick, this, [this]() { stop();  });
  connect(mView.get(), &View::videoItemDoubleClicked, this, [this](const QUrl url) 
  {
    const auto videos = mPlaylist.GetVideos();
    auto it = std::find(videos.begin(), videos.end(), url);
    if (it == videos.end())
    {
      return;
    }
    stop();

    mPlaylist.setCurrentIndex(mPlaylist.indexOf(*it));
    mPlayer->setVideo(mPlaylist.current());
    mSequenceMap.clear();
    mView->setSequences(mSequenceMap);

    play();
  });
  connect(mView.get(), &View::speedChanged, this, [this](double speed) { mPlayer->setPlaybackRate(speed); });
  connect(mView.get(), &View::volumeChanged, this, [this](double volume) { mPlayer->setVolume(static_cast<float>(volume)); mSettings.mVolume = static_cast<float>(volume); });
  connect(mView.get(), &View::sequenceSelected, this, [ this ](const Sequence* wSequence) { 
    wSequence == nullptr ? 
      mView->setDurationLabel(mPlayer->getDuration()) 
      : mView->setDurationLabel(wSequence->second - wSequence->first, true); // TODO solve the coloring of the label in a better way
    mSelectedSequence = wSequence; });
  connect(mView.get(), &View::sequenceDoubleClicked, this, [ this ](const Sequence* wSequence) 
  {
    qDebug() << "double clicked sequence" << wSequence->first.ms() << wSequence->second.ms();
    QString wFileName = QFileInfo(mPlaylist.current().toString()).fileName();

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

  mPlayer->setVolume(0.0f);
  mPlayer->setPlaybackRate(1.0);
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
  mPlaylist.setOrder(mSettings.mRandomize);

  mPlayer->setVideo(mPlaylist.current());

  mSequenceMap.clear();
  mView->setSequences(mSequenceMap);  // TODO: store the sequences associated to the video, not the player, so that we can have different sequences for each video in the playlist
  mView->setVideoList(mPlaylist.GetVideos());
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
  mView->setVolume(mSettings.mVolume);
  mView->setRandomize(mSettings.mRandomize);
  mPlaylist.setOrder(mSettings.mRandomize);
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
  mView->onPlay();
  mPlaying = true;

  SetThreadExecutionState(ES_CONTINUOUS | ES_DISPLAY_REQUIRED | ES_SYSTEM_REQUIRED);
}

void MediaPlayer::pause()
{
  mPlayer->pause();
  mPlaying = false;
  
  mView->onPause();
  
  SetThreadExecutionState(ES_CONTINUOUS);
}

void MediaPlayer::stop()
{
  mPlayer->stop();
  mView->onStop();
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
    mPlaylist.next();

    stop();
    mPlayer->setVideo(mPlaylist.current());
    mView->setCurrentVideo(static_cast<int>(mPlaylist.currentIndex()));
    mSequenceMap.clear();
    mSelectedSequence = nullptr;
    mView->setSequences(mSequenceMap);  // TODO: store the sequences associated to the video, ...
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
    mPlaylist.previous();

    stop();
    mPlayer->setVideo(mPlaylist.current());
    mView->setCurrentVideo(static_cast<int>(mPlaylist.currentIndex()));
    mSequenceMap.clear();
    mSelectedSequence = nullptr;
    mView->setSequences(mSequenceMap);  // TODO: store the sequences associated to the video, ...
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

void MediaPlayer::setFullscreen(const bool fullscreen)
{
  mView->setFullscreenView(fullscreen);
}

bool MediaPlayer::isFullscreen() const
{
  return mView->isFullscreenView();
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
      wStepSize = VTime(utils::Random(wMin, wMax));  // assuming 10 ms is left from the video, at least..
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
    mView->setSequences(mSequenceMap);
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
    if (wSequenceEntryIt == mSequenceMap.end())
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
  const QFileInfo fileInfo(mPlaylist.current().toLocalFile());
  const QString info(fileInfo.completeBaseName() + " - " + QString::number(mPlayer->getMetadata().value(QMediaMetaData::Resolution).value<QSize>().width()) + " x " + QString::number(mPlayer->getMetadata().value(QMediaMetaData::Resolution).value<QSize>().height()));
  mView->setInfo(info);
  mView->setCurrentVideo(static_cast<int>(mPlaylist.currentIndex()));

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
  const QString& wVideoPath = mPlaylist.current().toLocalFile();

  const QString wCutFilePath = mOutputRootDirectory + utils::prettifyFileName(QFileInfo(wVideoPath).completeBaseName()) + "." + wStartTime.toString('.') + "." + QString::number((wEndTime - wStartTime).ms()) + ".mp4";
  sequenceEntry.second.mFilePath = wCutFilePath;

  mProcesses.push_back(std::make_unique<QProcess>(this));
  QProcess* wProcess = mProcesses.back().get();

  connect(wProcess, &QProcess::started, this, [ & ]() {
    sequenceEntry.second.mState = OperationState::Processing;
    mView->setSequences(mSequenceMap);
    logStatusMessage("Fast cut started");
  });

  connect(wProcess, &QProcess::finished, this, [ & ](int exitCode, QProcess::ExitStatus exitStatus) {
    sequenceEntry.second.mState = exitCode == 0 ? OperationState::Succeeded : OperationState::Failed;
    mView->setSequences(mSequenceMap);
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
          mView->setSequences(mSequenceMap);
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
            mView->setSequences(mSequenceMap);
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
  const QString& wVideoPath = mPlaylist.current().toLocalFile();

  const QString wCutFilePath = mOutputRootDirectory + utils::prettifyFileName(QFileInfo(wVideoPath).completeBaseName()) + "." + wStartTime.toString('.') + "." + QString::number((wEndTime - wStartTime).ms()) + ".mp4";
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
    args.append({ "-c:a", "aac", wCutFilePath });
  }

  mProcesses.push_back(std::make_unique<QProcess>(this));
  QProcess* wProcess = mProcesses.back().get();
  connect(wProcess, &QProcess::started, this, [&]() {
    sequenceEntry.second.mState = OperationState::Processing;
    mView->setSequences(mSequenceMap);
    logStatusMessage(QString("Precise cut started on ") + (mGpuEncode ? "GPU" : "CPU") + (mDeinterlace ? " with deinterlacing" : ""));
    });

  connect(wProcess, &QProcess::finished, this, [&](int exitCode, QProcess::ExitStatus exitStatus) {
    sequenceEntry.second.mState = exitCode == 0 ? OperationState::Succeeded : OperationState::Failed;
    mView->setSequences(mSequenceMap);
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
          mView->setSequences(mSequenceMap);
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
          mView->setSequences(mSequenceMap);
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

  const QString wPrettyFileName = utils::prettifyFileName(QFileInfo(mPlaylist.current().toLocalFile()).completeBaseName());
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
    mView->setSequences(mSequenceMap);
    logStatusMessage(QString("Loop cut started on ") + (mGpuEncode ? "GPU" : "CPU") + (mDeinterlace ? " with deinterlacing" : ""));
    });

  connect(wCutProcess, &QProcess::finished, this, [&, wCutFilePath, wLoopFilePath, loopCount](int exitCode, QProcess::ExitStatus exitStatus) {
    logStatusMessage(QString("Precise cut ") + (exitCode == 0 ? "succeeded" : "failed"));
    if (exitCode != 0)
    {
      sequenceEntry.second.mState = OperationState::Failed;
      mView->setSequences(mSequenceMap);
      return;
    }

    const QString wReversedFilePath = QFileInfo(wCutFilePath).absolutePath() + QFileInfo(wCutFilePath).completeBaseName() + "_reversed.mp4";

    // Execute reverser process
    mProcesses.push_back(std::make_unique<QProcess>(this));
    QProcess* wReverserProcess = mProcesses.back().get();
    connect(wReverserProcess, &QProcess::started, this, [&]() {
      sequenceEntry.second.mProcessTimer = VTime(0);
      mView->setSequences(mSequenceMap);
      logStatusMessage("Reverser started");
      });

    connect(wReverserProcess, &QProcess::finished, this, [&, wCutFilePath, wLoopFilePath, wReversedFilePath, loopCount](int exitCode, QProcess::ExitStatus exitStatus) {
      logStatusMessage(QString("Reverser ") + (exitCode == 0 ? "succeeded" : "failed"));
      if (exitCode != 0)
      {
        sequenceEntry.second.mState = OperationState::Failed;
        mView->setSequences(mSequenceMap);
        return;
      }

      // Merge the files
      // Create concat list file
      const QString wConcatFilePath = utils::uniqueFileName(mOutputRootDirectory + "concat.txt");
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
        mView->setSequences(mSequenceMap);
        logStatusMessage("Merger started");
        });

      connect(wMergerProcess, &QProcess::finished, this, [&, wConcatFilePath, wCutFilePath, wReversedFilePath](int exitCode, QProcess::ExitStatus exitStatus) {
        sequenceEntry.second.mState = exitCode == 0 ? OperationState::Succeeded : OperationState::Failed;
        mView->setSequences(mSequenceMap);
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
              mView->setSequences(mSequenceMap);
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
              mView->setSequences(mSequenceMap);
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
            mView->setSequences(mSequenceMap);
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
            mView->setSequences(mSequenceMap);
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
          mView->setSequences(mSequenceMap);
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
          mView->setSequences(mSequenceMap);
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
          "-i", mPlaylist.current().toLocalFile(),
          "-ss", wPreloadTime.toString(),
      };
    }
    else
    {
      args = {
          "-i", mPlaylist.current().toLocalFile(),
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
    args.append({ "-c:a", "aac", wCutFilePath });
  }

  wCutProcess->start(mFFMpegPath, args);
}

void MediaPlayer::resetSeqenceState()
{
  for (auto& wSequence : mSequenceMap)
  {
    wSequence.second.mState = OperationState::Ready;
  }
  mView->setSequences(mSequenceMap);
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
  mView->setSequences(mSequenceMap);
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