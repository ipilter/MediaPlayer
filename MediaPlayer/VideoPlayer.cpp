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

  mVideoPlayer.reset(new QMediaPlayer(this));
  mVideoPlayer->setVideoOutput(videoWidget);
  mVideoPlayer->setAudioOutput(mAudioOutput);

  connect(mVideoPlayer.get(), &QMediaPlayer::positionChanged, this, [this](qint64 t) { emit positionChanged(VTime(t)); });
  connect(mVideoPlayer.get(), &QMediaPlayer::durationChanged, this, [this](qint64 d) { emit durationChanged(VTime(d)); });
  connect(mVideoPlayer.get(), &QMediaPlayer::mediaStatusChanged, this, [this](QMediaPlayer::MediaStatus status) {
    switch (status)
    {
      case QMediaPlayer::NoMedia:
      case QMediaPlayer::LoadingMedia:
      case QMediaPlayer::StalledMedia:
      case QMediaPlayer::BufferingMedia:
      case QMediaPlayer::BufferedMedia:
      case QMediaPlayer::InvalidMedia:
        break;
      case QMediaPlayer::EndOfMedia:
      {
        mVideoPlayer->pause();
        mVideoPlayer->setPosition(mVideoPlayer->duration());
        emit videoEnded();
        break;
      }
      case QMediaPlayer::LoadedMedia:
      {
        if (mIsVideoLoaded)
        {
          return;
        }
        mIsVideoLoaded = true;
        emit videoLoaded();
        break;
      }
      default:
        break;
    }
  });
}

void VideoPlayer::setVideo(const QUrl& videoUrl)
{
  if(isPlaying())
  {
    mVideoPlayer->stop();
  }
  mIsVideoLoaded = false;
  mVideoPlayer->setSource(videoUrl);
}

VTime VideoPlayer::getDuration() const
{
  return VTime(mVideoPlayer->duration());
}

VTime VideoPlayer::getPosition() const
{
  return VTime(mVideoPlayer->position());
}

bool VideoPlayer::isPlaying() const
{
  return mVideoPlayer->isPlaying();
}

void VideoPlayer::play()
{
  mVideoPlayer->play();
}

void VideoPlayer::pause()
{
  mVideoPlayer->pause();
}

void VideoPlayer::stop()
{
  mVideoPlayer->stop();
}

void VideoPlayer::seekBackward(VTime size)
{
  if (mVideoPlayer->position() <= size.ms())
  {
    mVideoPlayer->setPosition(0);
    return;
  }
  mVideoPlayer->setPosition(mVideoPlayer->position() - size.ms());
}

void VideoPlayer::seekForward(VTime size)
{
  if (mVideoPlayer->position() + size.ms() >= mVideoPlayer->duration())
  {
    mVideoPlayer->setPosition(mVideoPlayer->duration());
    return;
  }
  mVideoPlayer->setPosition(mVideoPlayer->position() + size.ms());
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
  mVideoPlayer->setPosition(position.ms());
}

void VideoPlayer::setPlaybackRate(qreal rate)
{
  mVideoPlayer->setPlaybackRate(rate);
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
  return mVideoPlayer->metaData();
}
