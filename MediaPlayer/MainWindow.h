#pragma once

#include "ui_MainWindow.h"
#include "MediaPlayer.h"

#include <QtWidgets/QMainWindow>

class QString;

class MainWindow : public QMainWindow
{
  Q_OBJECT

public:
  using Playlist = MediaPlayer::Playlist;

  MainWindow(QWidget *parent = nullptr);
  ~MainWindow();

  void setPlaylist(const Playlist& playlist);
  void setSettings(const MediaPlayer::Settings& settings);
  const MediaPlayer::Settings& getSettings() const;

private:
  void keyPressEvent(QKeyEvent* event);

private:
  Ui::MainWindowClass ui;

  std::shared_ptr<MediaPlayer> mMediaPlayer;
};
