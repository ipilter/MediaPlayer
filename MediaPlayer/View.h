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

  void addSequence(const Sequence& seqence);

  void setMuted(bool muted);
  void onPlay();
  void onPause();
  void onStop();

public slots:
  void setPosition(VTime position);
  void setDuration(VTime duration);
  void setInfo(const QString& info);  

signals:
  void sliderChanged(int position);
  void previousButtonClicked();
  void startStopButtonClicked();
  void nextButtonClicked();
  void muteButtonClicked();
  void onMouseClick();

private:
  VideoWidget* mVideoWidget;
  Slider* mSlider;
  QPushButton* mPreviousButton;
  QPushButton* mPlayButton;
  QPushButton* mNextButton;

  QLabel* mPositionLabel;
  QLabel* mDurationLabel;
  QPushButton* mMuteButton;
  
  QLabel* mInfoBarLabel;

  QLayout* mLayout;

  std::map<QString, QPixmap> mPixmapTable;
};
