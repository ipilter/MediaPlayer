#include "Player.h"
#include "View.h"
#include "VideoWidget.h"

#include <QMediaMetaData>
#include <random>

Player::Player(VideoWidget* videoWidget, QObject* parent)
  : QObject(parent)
{
  mMediaPlayer = new QMediaPlayer(this);
  mMediaPlayer->setVideoOutput(videoWidget);

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
  });
}

void Player::setVideo(const QUrl& videoUrl)
{
  mMediaPlayer->setSource(videoUrl);
}

quint64 Player::duration() const
{
  return mMediaPlayer->duration();
}

quint64 Player::position() const
{
  return mMediaPlayer->position();
}

bool Player::isPlaying() const
{
  return mMediaPlayer->isPlaying();
}

void Player::play()
{
  mMediaPlayer->play();
}

void Player::pause()
{
  mMediaPlayer->pause();
}

void Player::stop()
{
  mMediaPlayer->stop();
}

void Player::seekBackward(Time size)
{
  if (mMediaPlayer->position() <= size)
  {
    mMediaPlayer->setPosition(0);
    return;
  }
  mMediaPlayer->setPosition(mMediaPlayer->position() - size);
}

void Player::seekForward(Time size)
{
  if (mMediaPlayer->position() + size >= mMediaPlayer->duration())
  {
    mMediaPlayer->setPosition(mMediaPlayer->duration());
    return;
  }
  mMediaPlayer->setPosition(mMediaPlayer->position() + size);
}

void Player::setPosition(Time position)
{
  mMediaPlayer->setPosition(position);
}

QMediaMetaData Player::getMetadata() const
{
  return mMediaPlayer->metaData();
}
