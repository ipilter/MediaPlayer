#include "MediaPlayer.h"
#include "VideoPlayer.h"
#include "View.h"

#include <QFileInfo.h>
#include <QMediaMetadata.h>
#include <QUrl>

#include <random>

MediaPlayer::MediaPlayer(QObject* parent)
  : QObject(parent)
  , mView(std::make_shared<View>())
  , mPlayer(std::make_shared<VideoPlayer>(mView->getVideoWidget()))
{
  QObject::connect(mPlayer.get(), &VideoPlayer::positionChanged, this, [this](Time position) {mView->setPosition(position); });
  QObject::connect(mPlayer.get(), &VideoPlayer::durationChanged, this, [this](Time duration) {mView->setDuration(duration); });
  QObject::connect(mPlayer.get(), &VideoPlayer::videoLoaded, this, &MediaPlayer::onMediaLoaded);


  QObject::connect(mView.get(), &View::onMouseClick,           this, [this]() { mView->hide(); });
  QObject::connect(mView.get(), &View::sliderChanged,          this, [this](int position) {setPosition(static_cast<Time>(position)); });
  QObject::connect(mView.get(), &View::previousButtonClicked,  this, [this]() { previous(); });
  QObject::connect(mView.get(), &View::startStopButtonClicked, this, [this]() { startStop(); });
  QObject::connect(mView.get(), &View::nextButtonClicked,      this, [this]() { next(); });
  QObject::connect(mView.get(), &View::muteButtonClicked, this, [this]() { toggleMute(); });

  mPlayer->setVolume(50);
}

MediaPlayer::~MediaPlayer()
{}

void MediaPlayer::setPlaylist(const Playlist& playlist)
{
  mPlaylist = playlist;
  mCurrentVideo = 0;
  mPlayer->setVideo(mPlaylist[mCurrentVideo]);
}

QLayout* MediaPlayer::getLayout() const
{
  return mView->getLayout();
}

void MediaPlayer::setSettings(const Settings& settings)
{
  mSettings = settings;
  
  mPlayer->setMuted(mSettings.mMuted);
  mView->setMuted(mSettings.mMuted);
}

const MediaPlayer::Settings& MediaPlayer::getSettings() const
{
  return mSettings;
}

bool MediaPlayer::isPlaying() const
{
  return mPlayer->isPlaying();
}

void MediaPlayer::play()
{
  mPlayer->play();
  mView->setPlayButtonText("||");
}

void MediaPlayer::pause()
{
  mPlayer->pause();
  mView->setPlayButtonText("|>");
}

void MediaPlayer::stop()
{
  mPlayer->stop();
  mView->setPlayButtonText("|>");
}

void MediaPlayer::next()
{
  if (mCurrentVideo == mPlaylist.size() - 1)
  {
    mCurrentVideo = 0;
  }
  else
  {
    ++mCurrentVideo;
  }

  const bool isPlaying = mPlayer->isPlaying();
  stop();
  mPlayer->setVideo(mPlaylist[mCurrentVideo]);
  if (isPlaying)
  {
    play();
  }
}

void MediaPlayer::previous()
{
  if (mCurrentVideo == 0)
  {
    mCurrentVideo = mPlaylist.size() - 1;
  }
  else
  {
    --mCurrentVideo;
  }

  const bool isPlaying = mPlayer->isPlaying();
  stop();
  mPlayer->setVideo(mPlaylist[mCurrentVideo]);
  if (isPlaying)
  {
    play();
  }
}

void MediaPlayer::toggleMute()
{
  mSettings.mMuted = !mSettings.mMuted;

  mPlayer->setMuted(mSettings.mMuted);
  mView->setMuted(mSettings.mMuted);
}

void MediaPlayer::setPosition(Time position)
{
  mPlayer->setPosition(position);
}

void MediaPlayer::seekBackward(Time size)
{
  mPlayer->seekBackward(size);
}

void MediaPlayer::seekForward(Time size)
{
  mPlayer->seekForward(size);
}

void MediaPlayer::startStop()
{
  if (isPlaying())
  {
    pause();
  }
  else
  {
    play();
  }
}

void MediaPlayer::mark()
{
  if (mIsMarking)
  {
    mIsMarking = false;
    mSequences.back().second = mPlayer->getPosition();
    if (mSequences.back().first < mSequences.back().second)
    {
      mView->addSequence(mSequences.back());
    }
    else
    {
      mSequences.pop_back();
    }
    return;
  }

  mIsMarking = true;
  mSequences.push_back(std::make_pair(mPlayer->getPosition(), 0)); // use static instance for invalid sequence (min > max)
}

void MediaPlayer::cancelMark()
{
  mIsMarking = false;
  mSequences.pop_back();
}

void MediaPlayer::onMediaLoaded()
{ 
  const QFileInfo fileInfo(mPlaylist[mCurrentVideo].toLocalFile());
  const QString info(fileInfo.completeBaseName() + " - " + QString::number(mPlayer->getMetadata().value(QMediaMetaData::Resolution).value<QSize>().width()) + " x " + QString::number(mPlayer->getMetadata().value(QMediaMetaData::Resolution).value<QSize>().height()));
  mView->setInfo(info);

  setPosition(0);
  if (mSettings.mAutoPlay)
  {
    play();
  }
}
