#pragma once

#include "Types.h"

#include <QObject>
#include <QMediaMetaData>
#include <vector>

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

  Time getDuration() const;
  Time getPosition() const;

  bool isPlaying() const;

  void play();
  void pause();
  void stop();

  void seekBackward(Time size);
  void seekForward(Time size);

  float volume() const;
  bool isMuted() const;

  QMediaMetaData getMetadata() const;

public slots:
  void setPosition(Time position);
  void setPlaybackRate(qreal rate);
  void setVolume(float volume);
  void setMuted(bool muted);

signals:
  void positionChanged(Time position);
  void durationChanged(Time duration);
  void videoLoaded();

private:
  QMediaPlayer* mMediaPlayer;
  bool mIsVideoLoaded = false;

  QAudioOutput* mAudioOutput = nullptr;
};
