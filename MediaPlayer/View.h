#pragma once

#include "Types.h"

#include <QWidget>

class VideoWidget;
class Slider;
class QLabel;
class QPushButton;
class QLayout;

class VideoPlayer;

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

public slots:
  void setPosition(Time position);
  void setDuration(Time duration);
  void setInfo(const QString& info);
  void setPlayButtonText(const QString& text);

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
  QPushButton* mStartStopButton;
  QPushButton* mNextButton;

  QLabel* mPositionLabel;
  QLabel* mDurationLabel;
  QPushButton* mMuteButton;
  
  QLabel* mInfoBarLabel;

  QLayout* mLayout;
};
