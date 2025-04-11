#pragma once

#include <QWidget>
#include <memory>
#include <vector>

#include "Types.h"

class View;
class Player;

class MediaPlayer : public QObject
{
  Q_OBJECT

public:
  using Playlist = std::vector<QUrl>;
  struct Settings { bool mAutoPlay = false; };

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

  void setPosition(Time position);
  void seekBackward(Time size);
  void seekForward(Time size);
  void startStop();

  void mark();
  void cancelMark();

private:
  void onMediaLoaded();

private:
  std::shared_ptr<View> mView;
  std::shared_ptr<Player> mPlayer;

  // video management
  Playlist mPlaylist;
  size_t mCurrentVideo = 0;

  // sequence management
  Sequences mSequences;
  bool mIsMarking = false;

  // settings
  Settings mSettings;
};
