#include "View.h"
#include "VideoWidget.h"
#include "Slider.h"
#include "CursorHider.h"

#include <QVBoxLayout>
#include <QPushButton>
#include <QLabel>
#include <QTime>
#include <QWidget>
#include <QKeyEvent>
#include <QApplication>
#include <QPixmap>
#include <QPlainTextEdit>
#include <QSpinBox>
#include <QTextBlock>
#include <QScrollBar>

View::View(QWidget* parent)
  : QWidget(parent)
{
  mPixmapTable.emplace("play",     QPixmap(":/bitmaps/play.png"));
  mPixmapTable.emplace("pause",    QPixmap(":/bitmaps/pause.png"));
  mPixmapTable.emplace("previous", QPixmap(":/bitmaps/previous.png"));
  mPixmapTable.emplace("next",     QPixmap(":/bitmaps/next.png"));
  mPixmapTable.emplace("muted",    QPixmap(":/bitmaps/muted.png"));
  mPixmapTable.emplace("unmuted",  QPixmap(":/bitmaps/unmuted.png"));

  mVideoWidget = new VideoWidget(parent);
  mVideoWidget->setObjectName("videoWidget");

  mSlider = new Slider(Qt::Horizontal, parent);
  mSlider->setObjectName("videoSlider");

  mPreviousButton = new QPushButton(parent);
  mPreviousButton->setIcon(QIcon(mPixmapTable["previous"]));

  mPlayButton = new QPushButton(parent);
  mPlayButton->setIcon(QIcon(mPixmapTable["play"]));

  mNextButton = new QPushButton(parent);
  mNextButton->setIcon(QIcon(mPixmapTable["next"]));

  mMuteButton = new QPushButton(parent);
  mMuteButton->setIcon(QIcon(mPixmapTable["muted"]));

  mPositionLabel = new QLabel("00:00:00:000", parent);
  mPositionLabel->setObjectName("positionLabel");
  mPositionLabel->setAlignment(Qt::AlignCenter | Qt::AlignVCenter);

  mDurationLabel = new QLabel("00:00:00:000", parent);
  mDurationLabel->setObjectName("durationLabel");
  mDurationLabel->setAlignment(Qt::AlignCenter | Qt::AlignVCenter);

  mLoopCountSpinBox = new QSpinBox(parent);
  mLoopCountSpinBox->setObjectName("loopCountSpinBox");
  mLoopCountSpinBox->setRange(1, 999);
  mLoopCountSpinBox->setValue(7);
  mLoopCountSpinBox->setSingleStep(1);
  mLoopCountSpinBox->setPrefix("Loops: ");
  mLoopCountSpinBox->setButtonSymbols(QAbstractSpinBox::NoButtons);
  mLoopCountSpinBox->setAlignment(Qt::AlignCenter | Qt::AlignVCenter);
  mLoopCountSpinBox->setMinimumWidth(140);

  mInfoBar = new QPlainTextEdit(parent);
  mInfoBar->setObjectName("infoBar");
  mInfoBar->setReadOnly(true);
  mInfoBar->setMaximumHeight(45);
  mInfoBar->setLineWrapMode(QPlainTextEdit::NoWrap);
  mInfoBar->setHorizontalScrollBarPolicy(Qt::ScrollBarAlwaysOff);

  QHBoxLayout* buttonLayout = new QHBoxLayout;
  buttonLayout->addWidget(mPreviousButton);
  buttonLayout->addWidget(mPlayButton);
  buttonLayout->addWidget(mNextButton);
  buttonLayout->addWidget(mPositionLabel);
  buttonLayout->addWidget(mDurationLabel);
  buttonLayout->addWidget(mMuteButton);
  buttonLayout->addWidget(mLoopCountSpinBox);

  QVBoxLayout* rootLayout = new QVBoxLayout(parent);
  rootLayout->addWidget(mVideoWidget);
  rootLayout->addWidget(mSlider);
  rootLayout->addLayout(buttonLayout);
  rootLayout->addWidget(mInfoBar);
  mLayout = rootLayout;

  connect(mSlider, &QSlider::sliderMoved, this, [ this ](int position) { emit sliderChanged(position); });
  connect(mSlider, &Slider::sequenceSelected, this, [ this ](const Sequence* wSequence) { emit sequenceSelected(wSequence); });
  connect(mPreviousButton, &QPushButton::clicked, this, [ this ]() { emit previousButtonClicked(); mPlayButton->setFocus(); });
  connect(mPlayButton, &QPushButton::clicked, this, [ this ]() { emit startStopButtonClicked(); });
  connect(mNextButton, &QPushButton::clicked, this, [ this ]() { emit nextButtonClicked(); mPlayButton->setFocus(); });
  connect(mVideoWidget, &VideoWidget::mouseClicked, this, [ this ]() { emit onMouseClick(); mPlayButton->setFocus(); });
  connect(mMuteButton, &QPushButton::clicked, this, [ this ]() { emit muteButtonClicked(); mPlayButton->setFocus(); });
  connect(mLoopCountSpinBox, &QSpinBox::valueChanged, this, [ this ](int value) { mLoopCountSpinBox->setValue(value); mPlayButton->setFocus(); });

  mCursorHider.reset(new CursorHider(mVideoWidget));
}

View::~View()
{
  mCursorHider.release();
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

void View::setMarking(bool marking)
{
  // todo, change the view to show the current sequence begin and length instead of the video pos and duration
  // colors are hardcoded for now, should be in a config file
  if (marking)
  {
    mPositionLabel->setStyleSheet("QLabel { color: #5078FF; }");
  }
  else
  {
    mPositionLabel->setStyleSheet("QLabel { color: #AAAAAA; }");
  }
}

void View::setCursorTimeout(int timeoutMs)
{
  mCursorHider->setTimeout(timeoutMs);
}

unsigned View::getLoopCount() const
{
  return mLoopCountSpinBox->value();
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

void View::onSequencesChanged(const SequenceMap& sequences)
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

  setDurationLabel(duration);
}

void View::setDurationLabel(VTime duration, const bool isSequenceDuration)
{
  mDurationLabel->setText(duration.toString());
  if (isSequenceDuration)
  {
    mDurationLabel->setStyleSheet("QLabel { color: #B3A021; }");
  }
  else
  {
    mDurationLabel->setStyleSheet("QLabel { color: #AAAAAA; }");
  }
}

void View::setInfo(const QString& info)
{
  const QString wCurrentTimeStr = QTime::currentTime().toString("hh:mm:ss:zzz");
   mInfoBar->appendPlainText(wCurrentTimeStr + " - " + info);
   mInfoBar->verticalScrollBar()->setValue(mInfoBar->verticalScrollBar()->maximum()-1);
}
