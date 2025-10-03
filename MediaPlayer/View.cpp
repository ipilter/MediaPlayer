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
#include <QDoubleSpinBox>
#include <QTextBlock>
#include <QScrollBar>
#include <QCheckBox>

View::View(QWidget* parent)
  : QWidget(parent)
{
  mPixmapTable.emplace("play", QPixmap(":/bitmaps/play.png"));
  mPixmapTable.emplace("pause", QPixmap(":/bitmaps/pause.png"));
  mPixmapTable.emplace("seekLeft", QPixmap(":/bitmaps/seekLeft.png"));
  mPixmapTable.emplace("seekRight", QPixmap(":/bitmaps/seekRight.png"));
  mPixmapTable.emplace("previous", QPixmap(":/bitmaps/previous.png"));
  mPixmapTable.emplace("next", QPixmap(":/bitmaps/next.png"));
  mPixmapTable.emplace("mutedAudio", QPixmap(":/bitmaps/mutedAudio.png"));
  mPixmapTable.emplace("videoAudio", QPixmap(":/bitmaps/videoAudio.png"));
  mPixmapTable.emplace("musicAudio", QPixmap(":/bitmaps/musicAudio.png"));

  mVideoWidget = new VideoWidget(parent);
  mVideoWidget->setObjectName("videoWidget");

  mSlider = new Slider(Qt::Horizontal, parent);
  mSlider->setObjectName("videoSlider");

  mPreviousButton = new QPushButton(parent);
  mPreviousButton->setIcon(QIcon(mPixmapTable["previous"]));

  mSeekLeft = new QPushButton(parent);
  mSeekLeft->setIcon(QIcon(mPixmapTable["seekLeft"]));
  mSeekLeft->setObjectName("seekLeftButton");

  mPlayButton = new QPushButton(parent);
  mPlayButton->setIcon(QIcon(mPixmapTable["play"]));

  mSeekRight = new QPushButton(parent);
  mSeekRight->setIcon(QIcon(mPixmapTable["seekRight"]));
  mSeekRight->setObjectName("seekRightButton");

  mNextButton = new QPushButton(parent);
  mNextButton->setIcon(QIcon(mPixmapTable["next"]));

  mAudioButton = new QPushButton(parent);
  mAudioButton->setIcon(QIcon(mPixmapTable["muted"]));

  mDeinterlaceCheckBox = new QCheckBox(parent);
  mDeinterlaceCheckBox->setText("Deinterlace");
  mDeinterlaceCheckBox->setChecked(false);
  mDeinterlaceCheckBox->setObjectName("myCheckBox");

  mGpuEncodeCheckBox = new QCheckBox(parent);
  mGpuEncodeCheckBox->setText("GPU Encode");
  mGpuEncodeCheckBox->setChecked(false);
  mGpuEncodeCheckBox->setObjectName("gpuEncodeCheckBox");

  mPositionLabel = new QLabel("00:00:00:000", parent);
  mPositionLabel->setObjectName("positionLabel");
  mPositionLabel->setAlignment(Qt::AlignCenter | Qt::AlignVCenter);

  mDurationLabel = new QLabel("00:00:00:000", parent);
  mDurationLabel->setObjectName("durationLabel");
  mDurationLabel->setAlignment(Qt::AlignCenter | Qt::AlignVCenter);

  mLoopCountSpinBox = new QSpinBox(parent);
  mLoopCountSpinBox->setObjectName("loopCountSpinBox");
  mLoopCountSpinBox->setRange(1, 999);
  mLoopCountSpinBox->setValue(4);
  mLoopCountSpinBox->setSingleStep(1);
  mLoopCountSpinBox->setPrefix("Loops: ");
  mLoopCountSpinBox->setButtonSymbols(QAbstractSpinBox::NoButtons);
  mLoopCountSpinBox->setAlignment(Qt::AlignCenter | Qt::AlignVCenter);
  mLoopCountSpinBox->setMinimumWidth(140);

  mBurstLengthSpinBox = new QDoubleSpinBox(parent);
  mBurstLengthSpinBox->setObjectName("burstLengthSpinBox");
  mBurstLengthSpinBox->setRange(0.0, 20.0);
  mBurstLengthSpinBox->setValue(2.0);
  mBurstLengthSpinBox->setSingleStep(0.1);
  mBurstLengthSpinBox->setPrefix("Burst length: ");
  mBurstLengthSpinBox->setButtonSymbols(QAbstractSpinBox::NoButtons);
  mBurstLengthSpinBox->setAlignment(Qt::AlignCenter | Qt::AlignVCenter);
  mBurstLengthSpinBox->setMinimumWidth(140);

  mInfoBar = new QPlainTextEdit(parent);
  mInfoBar->setObjectName("infoBar");
  mInfoBar->setReadOnly(true);
  mInfoBar->setMaximumHeight(45);
  mInfoBar->setLineWrapMode(QPlainTextEdit::NoWrap);
  mInfoBar->setHorizontalScrollBarPolicy(Qt::ScrollBarAlwaysOff);

  QHBoxLayout* buttonLayout = new QHBoxLayout;
  buttonLayout->addWidget(mPreviousButton);
  buttonLayout->addWidget(mSeekLeft);
  buttonLayout->addWidget(mPlayButton);
  buttonLayout->addWidget(mSeekRight);
  buttonLayout->addWidget(mNextButton);
  buttonLayout->addWidget(mPositionLabel);
  buttonLayout->addWidget(mDurationLabel);
  buttonLayout->addWidget(mLoopCountSpinBox);
  buttonLayout->addWidget(mBurstLengthSpinBox);
  buttonLayout->addWidget(mAudioButton);
  buttonLayout->addWidget(mDeinterlaceCheckBox);
  buttonLayout->addWidget(mGpuEncodeCheckBox);

  QVBoxLayout* rootLayout = new QVBoxLayout(parent);
  rootLayout->addWidget(mVideoWidget);
  rootLayout->addWidget(mSlider);
  rootLayout->addLayout(buttonLayout);
  rootLayout->addWidget(mInfoBar);
  mLayout = rootLayout;

  connect(mSlider, &QSlider::sliderMoved, this, [this](int position) { emit sliderChanged(position); });
  connect(mSlider, &Slider::sequenceSelected, this, [this](const Sequence* wSequence) { emit sequenceSelected(wSequence); });
  connect(mSlider, &Slider::sequenceDoubleClicked, this, [this](const Sequence* wSequence) { emit sequenceDoubleClicked(wSequence); });
  connect(mPreviousButton, &QPushButton::clicked, this, [this]() { emit previousButtonClicked(); mPlayButton->setFocus(); });

  connect(mSeekLeft, &QPushButton::clicked, this, [this]() { emit seekLeftButtonClicked(); });
  connect(mPlayButton, &QPushButton::clicked, this, [this]() { emit startStopButtonClicked(); });
  connect(mSeekRight, &QPushButton::clicked, this, [this]() { emit seekRightButtonClicked(); });

  connect(mNextButton, &QPushButton::clicked, this, [this]() { emit nextButtonClicked(); mPlayButton->setFocus(); });
  connect(mVideoWidget, &VideoWidget::mouseClicked, this, [this]() { emit onMouseClick(); mPlayButton->setFocus(); });
  connect(mAudioButton, &QPushButton::clicked, this, [this]() { emit audioButtonClicked(); mPlayButton->setFocus(); });
  connect(mLoopCountSpinBox, &QSpinBox::valueChanged, this, [this](int value) { mPlayButton->setFocus(); });
  connect(mBurstLengthSpinBox, &QDoubleSpinBox::valueChanged, this, [this](double value) { mPlayButton->setFocus(); });

  connect(mDeinterlaceCheckBox, &QCheckBox::checkStateChanged, this, [this]() { emit deinterlaceChecked(mDeinterlaceCheckBox->checkState() == Qt::Checked); mPlayButton->setFocus(); });
  connect(mGpuEncodeCheckBox, &QCheckBox::checkStateChanged, this, [this]() { emit gpuEncodeChecked(mGpuEncodeCheckBox->checkState() == Qt::Checked); mPlayButton->setFocus(); });

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

void View::toggleAudio(const Settings::AudioMode iAudioMode)
{
  switch (iAudioMode)
  {
    case Settings::AudioMode::Muted:
    mAudioButton->setIcon(QIcon(mPixmapTable["mutedAudio"]));
    break;
    case Settings::AudioMode::Video:
    mAudioButton->setIcon(QIcon(mPixmapTable["videoAudio"]));
    break;
    case Settings::AudioMode::Music:
    mAudioButton->setIcon(QIcon(mPixmapTable["musicAudio"]));
    break;
  }
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

VTime View::getBurstLength() const
{
  return static_cast<VTime>(mBurstLengthSpinBox->value() * 1000.0);
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
