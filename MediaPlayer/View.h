#pragma once

#include "Types.h"
#include "Settings.h"

#include <QWidget>
#include <QUrl>

class VideoWidget;
class VideoPlayer;
class Slider;

class QLabel;
class QPushButton;
class QLayout;
class QPixmap;
class QPlainTextEdit;
class QSpinBox;
class QDoubleSpinBox;
class CursorHider;
class QCheckBox;
class QListWidget;
class QHBoxLayout;
class QLineEdit;

class View : public QWidget
{
  Q_OBJECT

public:
  View(QWidget* parent = nullptr);
  ~View();

  VideoWidget* getVideoWidget() const;
  QLayout* getLayout() const;

  void toggleAudio(const Settings::AudioMode iAudioMode);
  void setMarking(bool marking);
  void setCursorTimeout(int timeoutMs);
  void setVolume(float volume);
  void setRandomize(bool state);
  unsigned getLoopCount() const;
  VTime getBurstLength() const;
  void setCurrentVideo(const size_t idx);
  void setFullscreenView(const bool fullscreen);
  bool isFullscreenView() const;
  void setVideoList(const std::vector<QUrl>& videos);
  void setSequences(const SequenceMap& seqences);

  void focusPlayButton();
  void focusFilterEdit();

public slots:
  void setPosition(VTime position);
  void setDuration(VTime duration);
  void setDurationLabel(VTime duration, const bool isSequenceDuration = false);
  void setInfo(const QString& info);
  void onPlay();
  void onPause();
  void onStop();
  void onFilterEditChanged();

signals:
  void sliderChanged(int position);
  void previousButtonClicked();
  void seekLeftButtonClicked();
  void startStopButtonClicked();
  void seekRightButtonClicked();
  void nextButtonClicked();
  void audioButtonClicked();
  void deinterlaceChecked(bool state);
  void gpuEncodeChecked(bool state);
  void sequenceSelected(const Sequence* wSequence);
  void sequenceDoubleClicked(const Sequence* wSequence);
  void videoItemDoubleClicked(const QUrl url);
  void speedChanged(double speed);
  void volumeChanged(double volume);
  void randomizeChanged(bool state);
  void filterChanged(const QString& text);
  void FilterCommited();

  void onMouseClick();
  void onMouseDoubleClick();

private:
  std::map<QString, QPixmap> mPixmapTable;

  VideoWidget* mVideoWidget;
  QListWidget* mVideoList;
  QLineEdit* mFilterEdit;
  Slider* mSlider;

  QHBoxLayout* mButtonLayout;
  QPushButton* mPreviousButton;
  QPushButton* mSeekLeft;
  QPushButton* mPlayButton;
  QPushButton* mSeekRight;
  QPushButton* mNextButton;
  QPushButton* mAudioButton;
  QCheckBox* mDeinterlaceCheckBox;
  QCheckBox* mGpuEncodeCheckBox;
  QCheckBox* mRandomizeCheckBox;

  QLabel* mPositionLabel;
  QLabel* mDurationLabel;
  QSpinBox* mLoopCountSpinBox;
  QDoubleSpinBox* mBurstLengthSpinBox;
  QDoubleSpinBox* mSpeedSpinBox;
  QDoubleSpinBox* mVolumeSpinBox;

  QPlainTextEdit* mInfoBar;

  QLayout* mLayout;
  bool mIsFullscreenView = false;

  QWidget* mSidePanel = nullptr;
  QWidget* mControlsPanel = nullptr;

  std::unique_ptr<CursorHider> mCursorHider;

  Settings::AudioMode mCurrentAudioMode = Settings::AudioMode::Muted;
};
