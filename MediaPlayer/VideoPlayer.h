#pragma once

#include "Types.h"

#include <QObject>
#include <QMediaMetaData>

#include <vector>
#include <memory>

class VideoWidget;
class View;
class QMediaPlayer;
class QAudioOutput;
class QUrl;

class VideoPlayer : public QObject
{
  Q_OBJECT

public:
  explicit VideoPlayer(VideoWidget* videoWidget, QObject* parent = nullptr);

  void setVideo(const QUrl& videoUrl);

  VTime getDuration() const;
  VTime getPosition() const;

  bool isPlaying() const;

  void play();
  void pause();
  void stop();

  void seekBackward(VTime size);
  void seekForward(VTime size);

  float volume() const;
  bool isMuted() const;
  QSize videoDimensions() const;

  QMediaMetaData getMetadata() const;

public slots:
  void setPosition(VTime position, const bool updateNeeded = false);
  void setPlaybackRate(qreal rate);
  void setVolume(float volume);
  void setMuted(bool muted);

signals:
  void positionChanged(VTime position);
  void durationChanged(VTime duration);
  void videoEnded();
  void videoLoaded();

private:
  std::unique_ptr<QMediaPlayer> mVideoPlayer;
  std::unique_ptr<QMediaPlayer> mMusicPlayer;
  bool mIsVideoLoaded = false;

  QAudioOutput* mAudioOutput = nullptr;
};
