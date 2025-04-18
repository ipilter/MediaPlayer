#include "VideoPlayer.h"
#include "View.h"
#include "VideoWidget.h"

#include <QMediaMetaData>
#include <QMediaPlayer>
#include <QAudioOutput>
#include <QUrl>
#include <random>

VideoPlayer::VideoPlayer(VideoWidget* videoWidget, QObject* parent)
  : QObject(parent)
{
  mAudioOutput = new QAudioOutput(this);

  mMediaPlayer = new QMediaPlayer(this);
  mMediaPlayer->setVideoOutput(videoWidget);
  mMediaPlayer->setAudioOutput(mAudioOutput);

  connect(mMediaPlayer, &QMediaPlayer::positionChanged, this, [this](Time t) { emit positionChanged(t); });
  connect(mMediaPlayer, &QMediaPlayer::durationChanged, this, [this](Time d) { emit durationChanged(d); });
  connect(mMediaPlayer, &QMediaPlayer::mediaStatusChanged, this, [this](QMediaPlayer::MediaStatus status) {
    if (status == QMediaPlayer::LoadedMedia)
    {
      if (mIsVideoLoaded)
      {
        return;
      }

      mIsVideoLoaded = true;
      emit videoLoaded();
    }
    else if (status == QMediaPlayer::EndOfMedia)
    {
      emit videoEnded();
    }
    //  NoMedia,
    //  LoadingMedia,
    //  LoadedMedia,
    //  StalledMedia,
    //  BufferingMedia,
    //  BufferedMedia,
    //  EndOfMedia,
    //  InvalidMedia
  });
}

void VideoPlayer::setVideo(const QUrl& videoUrl)
{
  mMediaPlayer->setSource(videoUrl);
}

Time VideoPlayer::getDuration() const
{
  return mMediaPlayer->duration();
}

Time VideoPlayer::getPosition() const
{
  return mMediaPlayer->position();
}

bool VideoPlayer::isPlaying() const
{
  return mMediaPlayer->isPlaying();
}

void VideoPlayer::play()
{
  mMediaPlayer->play();
}

void VideoPlayer::pause()
{
  mMediaPlayer->pause();
}

void VideoPlayer::stop()
{
  mMediaPlayer->stop();
}

void VideoPlayer::seekBackward(Time size)
{
  if (mMediaPlayer->position() <= size)
  {
    mMediaPlayer->setPosition(0);
    return;
  }
  mMediaPlayer->setPosition(mMediaPlayer->position() - size);
}

void VideoPlayer::seekForward(Time size)
{
  if (mMediaPlayer->position() + size >= mMediaPlayer->duration())
  {
    mMediaPlayer->setPosition(mMediaPlayer->duration());
    return;
  }
  mMediaPlayer->setPosition(mMediaPlayer->position() + size);
}

float VideoPlayer::volume() const
{
  return mAudioOutput->volume();
}

bool VideoPlayer::isMuted() const
{
  return mAudioOutput->isMuted();
}



void VideoPlayer::setPosition(Time position)
{
  mMediaPlayer->setPosition(position);
}

void VideoPlayer::setPlaybackRate(qreal rate)
{
  mMediaPlayer->setPlaybackRate(rate);
}

void VideoPlayer::setVolume(float volume)
{
  mAudioOutput->setVolume(volume);
}

void VideoPlayer::setMuted(bool muted)
{
  mAudioOutput->setMuted(muted);
}

QMediaMetaData VideoPlayer::getMetadata() const
{
  return mMediaPlayer->metaData();
}
