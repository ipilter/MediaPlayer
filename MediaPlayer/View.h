#pragma once

#include "Types.h"

#include <QWidget>

class VideoWidget;
class VideoPlayer;
class Slider;

class QLabel;
class QPushButton;
class QLayout;
class QPixmap;

class View : public QWidget
{
  Q_OBJECT

public:
  View(QWidget * parent = nullptr);
  ~View();

  VideoWidget* getVideoWidget() const;
  QLayout* getLayout() const;

  void setMuted(bool muted);

public slots:
  void setPosition(VTime position);
  void setDuration(VTime duration);
  void setInfo(const QString& info);
  void onPlay();
  void onPause();
  void onStop();
  void onSequencesChanged(const SequenceMap& seqences);

signals:
  void sliderChanged(int position);
  void previousButtonClicked();
  void startStopButtonClicked();
  void nextButtonClicked();
  void muteButtonClicked();
  void onMouseClick();

private:
  std::map<QString, QPixmap> mPixmapTable;

  VideoWidget* mVideoWidget;
  Slider* mSlider;
  QPushButton* mPreviousButton;
  QPushButton* mPlayButton;
  QPushButton* mNextButton;
  QPushButton* mMuteButton;

  QLabel* mPositionLabel;
  QLabel* mDurationLabel;

  QLabel* mInfoBarLabel;

  QLayout* mLayout;
};
