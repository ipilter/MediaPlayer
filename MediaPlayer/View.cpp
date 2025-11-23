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
#include <QListWidget>
#include <QFileInfo>

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
  mVideoWidget->setFocusPolicy(Qt::NoFocus);

  mVideoList = new QListWidget(parent);
  mVideoList->setObjectName("videoList");
  mVideoList->setFixedWidth(300);
  mVideoList->setVerticalScrollBarPolicy(Qt::ScrollBarAsNeeded);
  mVideoList->setHorizontalScrollBarPolicy(Qt::ScrollBarAlwaysOff);
  mVideoList->setFocusPolicy(Qt::NoFocus);

  mSlider = new Slider(Qt::Horizontal, parent);
  mSlider->setObjectName("videoSlider");
  mSlider->setFocusPolicy(Qt::NoFocus);

  mPreviousButton = new QPushButton(parent);
  mPreviousButton->setMinimumWidth(70);
  mPreviousButton->setIcon(QIcon(mPixmapTable["previous"]));
  mPreviousButton->setFocusPolicy(Qt::NoFocus);

  mSeekLeft = new QPushButton(parent);
  mSeekLeft->setIcon(QIcon(mPixmapTable["seekLeft"]));
  mSeekLeft->setMinimumWidth(70);
  mSeekLeft->setObjectName("seekLeftButton");
  mSeekLeft->setFocusPolicy(Qt::NoFocus);

  mPlayButton = new QPushButton(parent);
  mPlayButton->setMinimumWidth(70);
  mPlayButton->setIcon(QIcon(mPixmapTable["play"]));
  mPlayButton->setFocusPolicy(Qt::StrongFocus);

  mSeekRight = new QPushButton(parent);
  mSeekRight->setMinimumWidth(70);
  mSeekRight->setIcon(QIcon(mPixmapTable["seekRight"]));
  mSeekRight->setObjectName("seekRightButton");
  mSeekRight->setFocusPolicy(Qt::NoFocus);

  mNextButton = new QPushButton(parent);
  mNextButton->setMinimumWidth(70);
  mNextButton->setIcon(QIcon(mPixmapTable["next"]));
  mNextButton->setFocusPolicy(Qt::NoFocus);

  mAudioButton = new QPushButton(parent);
  mAudioButton->setMinimumWidth(70);
  mAudioButton->setIcon(QIcon(mPixmapTable["muted"]));
  mAudioButton->setFocusPolicy(Qt::NoFocus);

  mDeinterlaceCheckBox = new QCheckBox(parent);
  mDeinterlaceCheckBox->setText("Deinterlace");
  mDeinterlaceCheckBox->setChecked(false);
  mDeinterlaceCheckBox->setObjectName("myCheckBox");
  mDeinterlaceCheckBox->setFocusPolicy(Qt::NoFocus);

  mGpuEncodeCheckBox = new QCheckBox(parent);
  mGpuEncodeCheckBox->setText("GPU Encode");
  mGpuEncodeCheckBox->setChecked(false);
  mGpuEncodeCheckBox->setObjectName("gpuEncodeCheckBox");
  mGpuEncodeCheckBox->setFocusPolicy(Qt::NoFocus);

  mRandomizeCheckBox = new QCheckBox(parent);
  mRandomizeCheckBox->setText("Randomize");
  mRandomizeCheckBox->setChecked(false);
  mRandomizeCheckBox->setObjectName("randomizeCheckBox");
  mRandomizeCheckBox->setFocusPolicy(Qt::NoFocus);

  mPositionLabel = new QLabel("00:00:00:000", parent);
  mPositionLabel->setObjectName("positionLabel");
  mPositionLabel->setAlignment(Qt::AlignCenter | Qt::AlignVCenter);
  mPositionLabel->setFocusPolicy(Qt::NoFocus);

  mDurationLabel = new QLabel("00:00:00:000", parent);
  mDurationLabel->setObjectName("durationLabel");
  mDurationLabel->setAlignment(Qt::AlignCenter | Qt::AlignVCenter);
  mDurationLabel->setFocusPolicy(Qt::NoFocus);

  mLoopCountSpinBox = new QSpinBox(parent);
  mLoopCountSpinBox->setObjectName("loopCountSpinBox");
  mLoopCountSpinBox->setRange(1, 999);
  mLoopCountSpinBox->setValue(4);
  mLoopCountSpinBox->setSingleStep(1);
  mLoopCountSpinBox->setPrefix("Loops: ");
  mLoopCountSpinBox->setButtonSymbols(QAbstractSpinBox::NoButtons);
  mLoopCountSpinBox->setAlignment(Qt::AlignCenter | Qt::AlignVCenter);
  mLoopCountSpinBox->setMinimumWidth(110);
  mLoopCountSpinBox->setFocusPolicy(Qt::NoFocus);

  mBurstLengthSpinBox = new QDoubleSpinBox(parent);
  mBurstLengthSpinBox->setObjectName("burstLengthSpinBox");
  mBurstLengthSpinBox->setRange(0.0, 20.0);
  mBurstLengthSpinBox->setValue(2.0);
  mBurstLengthSpinBox->setSingleStep(0.1);
  mBurstLengthSpinBox->setPrefix("Burst length: ");
  mBurstLengthSpinBox->setButtonSymbols(QAbstractSpinBox::NoButtons);
  mBurstLengthSpinBox->setAlignment(Qt::AlignCenter | Qt::AlignVCenter);
  mBurstLengthSpinBox->setMinimumWidth(170);
  mBurstLengthSpinBox->setFocusPolicy(Qt::NoFocus);

  mSpeedSpinBox = new QDoubleSpinBox(parent);
  mSpeedSpinBox->setObjectName("speedSpinBox");
  mSpeedSpinBox->setRange(0.1, 10.0);
  mSpeedSpinBox->setValue(1.0);
  mSpeedSpinBox->setSingleStep(0.05);
  mSpeedSpinBox->setPrefix("Speed: ");
  mSpeedSpinBox->setButtonSymbols(QAbstractSpinBox::NoButtons);
  mSpeedSpinBox->setAlignment(Qt::AlignCenter | Qt::AlignVCenter);
  mSpeedSpinBox->setMinimumWidth(130);
  mSpeedSpinBox->setFocusPolicy(Qt::NoFocus);

  mVolumeSpinBox = new QDoubleSpinBox(parent);
  mVolumeSpinBox->setObjectName("volumeSpinBox");
  mVolumeSpinBox->setRange(0.0, 1.0);
  mVolumeSpinBox->setValue(0.0);
  mVolumeSpinBox->setSingleStep(0.05);
  mVolumeSpinBox->setPrefix("Volume: ");
  mVolumeSpinBox->setButtonSymbols(QAbstractSpinBox::NoButtons);
  mVolumeSpinBox->setAlignment(Qt::AlignCenter | Qt::AlignVCenter);
  mVolumeSpinBox->setMinimumWidth(130);
  mVolumeSpinBox->setFocusPolicy(Qt::NoFocus);

  mInfoBar = new QPlainTextEdit(parent);
  mInfoBar->setObjectName("infoBar");
  mInfoBar->setReadOnly(true);
  mInfoBar->setFixedHeight(32);
  mInfoBar->setLineWrapMode(QPlainTextEdit::NoWrap);
  mInfoBar->setHorizontalScrollBarPolicy(Qt::ScrollBarAlwaysOff);
  mInfoBar->setFocusPolicy(Qt::NoFocus);

  mButtonLayout = new QHBoxLayout;
  mButtonLayout->addWidget(mPreviousButton);
  mButtonLayout->addWidget(mSeekLeft);
  mButtonLayout->addWidget(mPlayButton);
  mButtonLayout->addWidget(mSeekRight);
  mButtonLayout->addWidget(mNextButton);
  mButtonLayout->addWidget(mPositionLabel);
  mButtonLayout->addWidget(mDurationLabel);
  mButtonLayout->addWidget(mSpeedSpinBox);
  mButtonLayout->addWidget(mLoopCountSpinBox);
  mButtonLayout->addWidget(mBurstLengthSpinBox);
  mButtonLayout->addWidget(mAudioButton);
  mButtonLayout->addWidget(mVolumeSpinBox);
  mButtonLayout->addWidget(mDeinterlaceCheckBox);
  mButtonLayout->addWidget(mGpuEncodeCheckBox);
  mButtonLayout->addWidget(mRandomizeCheckBox);

  QHBoxLayout* videoLayout = new QHBoxLayout;
  videoLayout->addWidget(mVideoWidget);
  videoLayout->addWidget(mVideoList);

  QVBoxLayout* rootLayout = new QVBoxLayout(parent);
  rootLayout->addLayout(videoLayout);
  rootLayout->addWidget(mSlider);
  rootLayout->addLayout(static_cast<QLayout*>(mButtonLayout));
  rootLayout->addWidget(mInfoBar);
  mLayout = rootLayout;

  connect(mVideoList, &QListWidget::itemDoubleClicked, this, [this](QListWidgetItem* item) {
    if (!item)
      return;
    
    const QUrl wVideoUrl = item->data(Qt::UserRole).value<QUrl>();
    if (!wVideoUrl.isValid())
      return;

    emit videoItemDoubleClicked(wVideoUrl);
  });

  connect(mSlider, &QSlider::sliderMoved, this, [this](int position) { emit sliderChanged(position); });
  connect(mSlider, &Slider::sequenceSelected, this, [this](const Sequence* wSequence) { emit sequenceSelected(wSequence); });
  connect(mSlider, &Slider::sequenceDoubleClicked, this, [this](const Sequence* wSequence) { emit sequenceDoubleClicked(wSequence); });
  
  connect(mPreviousButton, &QPushButton::clicked, this, [this]() { emit previousButtonClicked(); });

  connect(mSeekLeft, &QPushButton::clicked, this, [this]() { emit seekLeftButtonClicked(); });
  connect(mPlayButton, &QPushButton::clicked, this, [this]() { emit startStopButtonClicked(); });
  connect(mSeekRight, &QPushButton::clicked, this, [this]() { emit seekRightButtonClicked(); });

  connect(mNextButton, &QPushButton::clicked, this, [this]() { emit nextButtonClicked(); });
  
  connect(mVideoWidget, &VideoWidget::mouseClicked, this, [this]() { emit onMouseClick(); });
  connect(mAudioButton, &QPushButton::clicked, this, [this]() { emit audioButtonClicked(); });

  connect(mDeinterlaceCheckBox, &QCheckBox::checkStateChanged, this, [this]() { emit deinterlaceChecked(mDeinterlaceCheckBox->checkState() == Qt::Checked); });
  connect(mGpuEncodeCheckBox, &QCheckBox::checkStateChanged, this, [this]() { emit gpuEncodeChecked(mGpuEncodeCheckBox->checkState() == Qt::Checked); });
  connect(mRandomizeCheckBox, &QCheckBox::checkStateChanged, this, [this]() { emit randomizeChanged(mRandomizeCheckBox->checkState() == Qt::Checked); });

  connect(mSpeedSpinBox, QOverload<double>::of(&QDoubleSpinBox::valueChanged), this, [this](double value) { emit speedChanged(value); });
  connect(mVolumeSpinBox, QOverload<double>::of(&QDoubleSpinBox::valueChanged), this, [this](double value) { emit volumeChanged(value); });

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
  mCurrentAudioMode = iAudioMode;
  switch (mCurrentAudioMode)
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

void View::setVolume(float volume)
{
  mVolumeSpinBox->setValue(static_cast<double>(volume));
}

void View::setRandomize(const bool isRandomized)
{
  mRandomizeCheckBox->setChecked(isRandomized);
}

void View::setCurrentVideo(const size_t idx)
{
  mVideoList->setCurrentRow(static_cast<int>(idx));
}

void View::setFullscreenView(const bool isFullscreen)
{
  mIsFullscreenView = isFullscreen;

  auto toggleLayoutWidgets = [this](QLayout* layout, bool visible) {
    if (!layout) return;
    for (int i = 0; i < layout->count(); ++i)
    {
      QLayoutItem* item = layout->itemAt(i);
      if (!item) continue;
      if (QWidget* w = item->widget())
      {
        if (visible) w->show(); else w->hide();
      }
      else if (QLayout* subLayout = item->layout())
      {
        // recurse one level deep (handles nested layouts)
        for (int j = 0; j < subLayout->count(); ++j)
        {
          QLayoutItem* subItem = subLayout->itemAt(j);
          if (!subItem) continue;
          if (QWidget* sw = subItem->widget())
          {
            if (visible) sw->show(); else sw->hide();
          }
        }
      }
    }
    };

  if (mIsFullscreenView)
  {
    mVideoList->hide();
    mSlider->hide();
    mInfoBar->hide();
    toggleLayoutWidgets(mButtonLayout, false);
  }
  else
  {
    mVideoList->show();
    mSlider->show();
    mInfoBar->show();
    toggleLayoutWidgets(mButtonLayout, true);
  }
}

bool View::isFullscreenView() const
{
  return mIsFullscreenView; // TODO internal state?
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

void View::setSequences(const SequenceMap& sequences)
{
  mSlider->setSequences(sequences);
}

void View::setVideoList(const std::vector<QUrl>& videos)
{
  mVideoList->clear();
  for (const auto& video : videos)
  {
    const QString displayName = QFileInfo(video.toLocalFile()).completeBaseName();

    QListWidgetItem* item = new QListWidgetItem(displayName);
    item->setData(Qt::UserRole, QVariant::fromValue(video));

    mVideoList->addItem(item);
  }
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
