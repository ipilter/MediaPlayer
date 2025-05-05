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

  connect(mMediaPlayer, &QMediaPlayer::positionChanged, this, [this](qint64 t) { emit positionChanged(VTime(t)); });
  connect(mMediaPlayer, &QMediaPlayer::durationChanged, this, [this](qint64 d) { emit durationChanged(VTime(d)); });
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
      mMediaPlayer->pause();
      mMediaPlayer->setPosition(mMediaPlayer->duration());
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
  if(isPlaying())
  {
    mMediaPlayer->stop();
  }
  mIsVideoLoaded = false;
  mMediaPlayer->setSource(videoUrl);
}

VTime VideoPlayer::getDuration() const
{
  return VTime(mMediaPlayer->duration());
}

VTime VideoPlayer::getPosition() const
{
  return VTime(mMediaPlayer->position());
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

void VideoPlayer::seekBackward(VTime size)
{
  if (mMediaPlayer->position() <= size.ms())
  {
    mMediaPlayer->setPosition(0);
    return;
  }
  mMediaPlayer->setPosition(mMediaPlayer->position() - size.ms());
}

void VideoPlayer::seekForward(VTime size)
{
  if (mMediaPlayer->position() + size.ms() >= mMediaPlayer->duration())
  {
    mMediaPlayer->setPosition(mMediaPlayer->duration());
    return;
  }
  mMediaPlayer->setPosition(mMediaPlayer->position() + size.ms());
}

float VideoPlayer::volume() const
{
  return mAudioOutput->volume();
}

bool VideoPlayer::isMuted() const
{
  return mAudioOutput->isMuted();
}



void VideoPlayer::setPosition(VTime position)
{
  mMediaPlayer->setPosition(position.ms());
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
