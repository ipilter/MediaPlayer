#pragma once

#include "Types.h"

#include <QObject>
#include <memory>
#include <vector>

class View;
class VideoPlayer;
class QLayout;
class FFmpeg;

class MediaPlayer : public QObject
{
  Q_OBJECT

public:
  using Playlist = std::vector<QUrl>;
  struct Settings { bool mAutoPlay = false; bool mMuted = true; };

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
  void toggleMute();

  void setPosition(Time position);
  void seekBackward(Time size);
  void seekForward(Time size);
  void startStop();

  void mark();
  void cancelMark();
  void cut(const bool reconvert);

private:
  void onVideoLoaded();
  void onVideoEnded();

private:
  std::shared_ptr<View> mView;
  std::shared_ptr<VideoPlayer> mPlayer;
  std::unique_ptr<FFmpeg> mFFmpeg;

  // video management
  Playlist mPlaylist;
  size_t mCurrentVideo = 0;

  // sequence management
  Sequences mSequences;
  bool mIsMarking = false;

  // settings
  Settings mSettings;
};
