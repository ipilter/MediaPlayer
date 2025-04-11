#pragma once

#include "Types.h"

#include <QMediaPlayer>
#include <QObject>
#include <QUrl>
#include <QString>
#include <vector>

class VideoWidget;
class View;

class Player : public QObject
{
  Q_OBJECT

public:
  explicit Player(VideoWidget* videoWidget, QObject* parent = nullptr);

  void setVideo(const QUrl& videoUrl);

  quint64 duration() const;
  quint64 position() const;

  bool isPlaying() const;

  void play();
  void pause();
  void stop();

  void seekBackward(Time size);
  void seekForward(Time size);

  QMediaMetaData getMetadata() const;

public slots:
  void setPosition(Time position);

signals:
  void positionChanged(Time position);
  void durationChanged(Time duration);
  void videoLoaded();

private:
  QMediaPlayer* mMediaPlayer;
  bool mIsVideoLoaded = false;
};
