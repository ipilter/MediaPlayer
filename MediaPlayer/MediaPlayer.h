#pragma once

#include "Types.h"
#include "ProcessTree.h"

#include <QObject>

#include <memory>
#include <vector>

class View;
class VideoPlayer;
class QLayout;

class MediaPlayer : public QObject
{
  Q_OBJECT

public:
  using Playlist = std::vector<QUrl>;
  struct Settings { bool mAutoPlay = false; bool mMuted = true; };
  enum class CutMethod { Fast, Precise, Loop };

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

  void setPosition(VTime position);
  void seekBackward(VTime size);
  void seekForward(VTime size);
  void startStop();

  void mark();
  void cancelMark();
  void cut(const CutMethod cutMethod);

signals:
  void sequencesChanged(const SequenceMap& sequences);

private:
  void onVideoLoaded();
  void onVideoEnded();

private:
  std::shared_ptr<View> mView;
  std::shared_ptr<VideoPlayer> mPlayer;
  bool mPlaying = false;

  // video management
  Playlist mPlaylist;
  size_t mCurrentVideo = 0;

  // sequence management
  SequenceMap mSequenceMap; // TODO: store attributes, like cut state, video associated, etc.
  Sequence mEditedSequence = {0,0};

  ProcessTreeNode::Ptr mCutProcess; // TODO: make a pool of these and a manager

  // settings
  Settings mSettings;
};
