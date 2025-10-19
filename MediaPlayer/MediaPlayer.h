#pragma once

#include "Types.h"
#include "Process.h"
#include "Settings.h"
#include "Playlist.h"
#include "Filter.h"

#include <QObject>

#include <memory>

class View;
class VideoPlayer;
class QLayout;
class QProcess;
class QString;

class MediaPlayer : public QObject
{
  Q_OBJECT

public:
  enum class CutMethod { Fast, Precise, Loop };
  enum class SeekStep { Normal, Small, Big, Random };
  enum class SeekDirection { Forward, Backward };
  enum class SnapPosition { Start, End };

  using Playlist = details::Playlist;
  using Playlists = std::vector<Playlist>;

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
  void setDeinterlace(const bool state);
  void setGpuEncode(const bool state);

  void setPosition(const VTime& position, const bool updateNeeded = false);
  VTime getPosition() const;
  void seek(SeekDirection direction, SeekStep step);
  void snapToSelection(SnapPosition position);
  void startStop();

  void mark(const bool isCancel = false);
  void cut(const CutMethod cutMethod);

  // sequence management
  void resetSeqenceState();
  void deleteSequence();

  void logStatusMessage(const QString& msg);

  // TODO HACK !
  void burstCut();

signals:
  void sequencesChanged(const SequenceMap& sequences);
  void videoListChanged(const std::vector<QUrl>& videos);

private:
  void onVideoLoaded();
  void onVideoEnded();

  void FastCut(SequenceEntry& sequenceEntry);
  void PreciseCut(SequenceEntry& sequenceEntry);
  void LoopCut(SequenceEntry& sequenceEntry);

private:
  // controller data
  std::shared_ptr<View> mView;
  std::shared_ptr<VideoPlayer> mPlayer;
  bool mPlaying = false;
  bool mDeinterlace = false;
  bool mGpuEncode = false;

  // video management
  Playlist mPlaylist;
  size_t mCurrentVideo = 0;

  // sequence management
  SequenceMap mSequenceMap;
  Sequence mEditedSequence = Sequence{ VTime(0), VTime(0) };
  Sequence const* mSelectedSequence = nullptr;

  using ProcessPtr = std::unique_ptr<QProcess>;
  std::vector<ProcessPtr> mProcesses; // TODO: destroy finished processsed, do not accumulate them

  const QString mFFMpegPath = "d:\\Tools\\ffmpeg\\ffmpeg.exe"; // TODO: settings
  const QString mOutputRootDirectory = "a:\\";  // TODO: settings

  // settings
  Settings mSettings;
};
