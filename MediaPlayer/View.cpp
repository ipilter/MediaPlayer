#include "VideoWidget.h"
#include "View.h"
#include "Slider.h"

#include <QVBoxLayout>
#include <QPushButton>
#include <QLabel>
#include <QTime>
#include <QWidget>
#include <QKeyEvent>
#include <QApplication>
#include <QPixmap>

#include <random>

View::View(QWidget* parent)
  : QWidget(parent)
{
  mPixmapTable.emplace("play", QPixmap(":/bitmaps/play.png"));
  mPixmapTable.emplace("pause", QPixmap(":/bitmaps/pause.png"));
  mPixmapTable.emplace("previous", QPixmap(":/bitmaps/previous.png"));
  mPixmapTable.emplace("next", QPixmap(":/bitmaps/next.png"));
  mPixmapTable.emplace("muted", QPixmap(":/bitmaps/muted.png"));
  mPixmapTable.emplace("unmuted", QPixmap(":/bitmaps/unmuted.png"));

  mVideoWidget = new VideoWidget(parent);
  mVideoWidget->setFocus();

  mSlider = new Slider(Qt::Horizontal, parent);
  mSlider->setFixedHeight(50);

  mPreviousButton = new QPushButton(parent);
  mPlayButton = new QPushButton(parent);
  mNextButton = new QPushButton(parent);
  mMuteButton = new QPushButton(parent);
  mPositionLabel = new QLabel("00:00:00:000", parent);
  mPositionLabel->setObjectName("positionLabel");
  mDurationLabel = new QLabel("00:00:00:000", parent);
  mDurationLabel->setObjectName("durationLabel");

  mPreviousButton->setFixedHeight(35);
  mPlayButton->setFixedHeight(35);
  mNextButton->setFixedHeight(35);
  mMuteButton->setFixedHeight(35);
  mPositionLabel->setFixedHeight(35);
  mDurationLabel->setFixedHeight(35);

  QHBoxLayout* buttonLayout = new QHBoxLayout;
  buttonLayout->addWidget(mPreviousButton);
  buttonLayout->addWidget(mPlayButton);
  buttonLayout->addWidget(mNextButton);
  buttonLayout->addWidget(mPositionLabel);
  buttonLayout->addWidget(mDurationLabel);
  buttonLayout->addWidget(mMuteButton);

  mInfoBarLabel = new QLabel(parent);
  mInfoBarLabel->setObjectName("infoBarLabel");
  mInfoBarLabel->setFixedHeight(25);
  
  QVBoxLayout* rootLayout = new QVBoxLayout;

  rootLayout->addWidget(mVideoWidget);
  rootLayout->addWidget(mSlider);
  rootLayout->addLayout(buttonLayout);
  rootLayout->addWidget(mInfoBarLabel);
  mLayout = rootLayout;

  connect(mSlider, &QSlider::sliderMoved, this, [this](int position) { emit sliderChanged(position); });
  connect(mPreviousButton, &QPushButton::clicked, this, [this]() { emit previousButtonClicked(); });
  connect(mPlayButton, &QPushButton::clicked, this, [this]() { emit startStopButtonClicked(); });
  connect(mNextButton, &QPushButton::clicked, this, [this]() { emit nextButtonClicked(); });
  connect(mVideoWidget, &VideoWidget::mouseClicked, this, [this]() { emit onMouseClick(); });
  connect(mMuteButton, &QPushButton::clicked, this, [this]() { emit muteButtonClicked(); });

  mPlayButton->setIcon(QIcon(mPixmapTable["play"]));
  mPreviousButton->setIcon(QIcon(mPixmapTable["previous"]));
  mNextButton->setIcon(QIcon(mPixmapTable["next"]));
  mMuteButton->setIcon(QIcon(mPixmapTable["muted"]));
}

View::~View()
{
  delete mVideoWidget;
}

VideoWidget* View::getVideoWidget() const
{
  return mVideoWidget;
}

QLayout* View::getLayout() const
{
  return mLayout;
}

void View::setMuted(bool muted)
{
  mMuteButton->setIcon(muted ? mPixmapTable["muted"] : mPixmapTable["unmuted"]);
}

void View::onPlay()
{
   mPlayButton->setIcon(QIcon(mPixmapTable["pause"]));
}

void View::onPause()
{
  mPlayButton->setIcon(QIcon(mPixmapTable["play"]));
}

void View::onStop()
{
  mPlayButton->setIcon(QIcon(mPixmapTable["play"]));
}

void View::onSequencesChanged(const Sequences& sequences)
{
  mSlider->setSequences(sequences);
}

void View::setPosition(VTime position)
{
  mSlider->setValue(position.ms());

  QTime currentTime(0, 0, 0, 0);
  currentTime = currentTime.addMSecs(position.ms());
  mPositionLabel->setText(currentTime.toString("hh:mm:ss:zzz"));
}

void View::setDuration(VTime duration)
{
  mSlider->setRange(0, duration.ms());

  QTime durationTime(0, 0, 0, 0);
  durationTime = durationTime.addMSecs(duration.ms());
  mDurationLabel->setText(durationTime.toString("hh:mm:ss:zzz"));
}

void View::setInfo(const QString& info)
{
  mInfoBarLabel->setText(info);
}
