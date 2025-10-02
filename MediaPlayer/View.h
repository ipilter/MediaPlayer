#pragma once

#include "Types.h"
#include "Settings.h"

#include <QWidget>

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

class View : public QWidget
{
  Q_OBJECT

public:
  View(QWidget * parent = nullptr);
  ~View();

  VideoWidget* getVideoWidget() const;
  QLayout* getLayout() const;

  void toggleAudio(const Settings::AudioMode iAudioMode);
  void setMarking(bool marking);
  void setCursorTimeout(int timeoutMs);
  unsigned getLoopCount() const;
  VTime getBurstLength() const;

public slots:
  void setPosition(VTime position);
  void setDuration(VTime duration);
  void setDurationLabel(VTime duration, const bool isSequenceDuration = false);
  void setInfo(const QString& info);
  void onPlay();
  void onPause();
  void onStop();
  void onSequencesChanged(const SequenceMap& seqences);

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

  void onMouseClick();
  void onMouseDoubleClick();

private:
  std::map<QString, QPixmap> mPixmapTable;

  VideoWidget* mVideoWidget;
  Slider* mSlider;
  QPushButton* mPreviousButton;
  QPushButton* mSeekLeft;
  QPushButton* mPlayButton;
  QPushButton* mSeekRight;
  QPushButton* mNextButton;
  QPushButton* mAudioButton;
  QCheckBox* mDeinterlaceCheckBox;
  QCheckBox* mGpuEncodeCheckBox;

  QLabel* mPositionLabel;
  QLabel* mDurationLabel;
  QSpinBox* mLoopCountSpinBox;
  QDoubleSpinBox* mBurstLengthSpinBox;

  QPlainTextEdit* mInfoBar;

  QLayout* mLayout;

  std::unique_ptr<CursorHider> mCursorHider;

};
