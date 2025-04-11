#pragma once

#include <QtWidgets/QMainWindow>

#include <QMediaPlayer>
#include <QVideoWidget>
#include <QString>

#include "ui_MainWindow.h"
#include "Player.h"

#include "MediaPlayer.h"

class MainWindow : public QMainWindow
{
  Q_OBJECT

public:
  using Playlist = MediaPlayer::Playlist;

  MainWindow(QWidget *parent = nullptr);
  ~MainWindow();

  void applyStylesFromFile(const QString& filePath);
  void setPlaylist(const Playlist& playlist);
  void setSettings(const MediaPlayer::Settings& settings);
  const MediaPlayer::Settings& getSettings() const;

private:
  void keyPressEvent(QKeyEvent* event);

private:
  Ui::MainWindowClass ui;

  std::shared_ptr<MediaPlayer> mMediaPlayer;
};
