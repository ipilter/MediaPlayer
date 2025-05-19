#pragma once

#include "Types.h"
#include "Process.h"
#include "Settings.h"

#include <QObject>

#include <memory>
#include <vector>

class View;
class VideoPlayer;
class QLayout;
class QProcess;
class QString;

class MediaPlayer : public QObject
{
  Q_OBJECT

public:
  using Playlist = std::vector<QUrl>;

  enum class CutMethod { Fast, Precise, Loop };
  enum class SeekStep { Normal, Small, Big, Random };
  enum class SeekDirection { Forward, Backward };
  enum class SnapPosition { Start, End };

  MediaPlayer(QObject* parent = nullptr);
  ~MediaPlayer();

  void setPlaylist(const Playlist& playlist);
  QLayout* getLayout() const;

  void setSettings(const Settings& settings);
  const Settings& getSettings() const;

  bool isPlaying() const;
  void play();
  void pause();
  void stop();
  void next();
  void previous();
  void toggleAudio();

  void setPosition(const VTime& position);
  void seek(SeekDirection direction, SeekStep step);
  void snapToSelection(SnapPosition position);
  void startStop();

  void mark(const bool isCancel = false);
  void cut(const CutMethod cutMethod);

  // TODO better sequence management
  void resetSeqenceState();
  void deleteSequence();

  void logStatusMessage(const QString& msg);

signals:
  void sequencesChanged(const SequenceMap& sequences);

private:
  void onVideoLoaded();
  void onVideoEnded();

  void FastCut(SequenceEntry& sequenceEntry);
  void PreciseCut(SequenceEntry& sequenceEntry);
  void LoopCut(SequenceEntry& sequenceEntry);

private:
  std::shared_ptr<View> mView;
  std::shared_ptr<VideoPlayer> mPlayer;
  bool mPlaying = false;

  // video management
  Playlist mPlaylist;
  size_t mCurrentVideo = 0;

  // sequence management
  SequenceMap mSequenceMap;
  Sequence mEditedSequence = Sequence{ VTime(0), VTime(0) };
  Sequence const * mSelectedSequence = nullptr;

  using ProcessPtr = std::unique_ptr<QProcess>;
  std::vector<ProcessPtr> mProcesses; // TODO: destroy finished processsed, do not accumulate them
  
  const QString mFFMpegPath = "d:\\Tools\\ffmpeg\\ffmpeg.exe"; // TODO: settings
  const QString mOutputRootDirectory = "e:\\";  // TODO: settings

  // settings
  Settings mSettings;
};
