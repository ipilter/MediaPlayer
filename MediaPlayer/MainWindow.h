#pragma once

#include "ui_MainWindow.h"
#include "Playlist.h"
#include "MediaPlayer.h"

#include <QtWidgets/QMainWindow>

class QString;
struct Settings;

class MainWindow : public QMainWindow
{
  Q_OBJECT

public:
  MainWindow(QWidget* parent = nullptr);
  ~MainWindow();

  void setPlaylist(const Playlist& playlist);
  void setSettings(const Settings& settings);
  const Settings& getSettings() const;

  const Placement& getPlacement() const;

protected:
  virtual void keyPressEvent(QKeyEvent* event);
  virtual void resizeEvent(QResizeEvent* event);
  virtual void moveEvent(QMoveEvent* event);
  virtual void closeEvent(QCloseEvent* event);
  void dragEnterEvent(QDragEnterEvent* event) override;
  void dropEvent(QDropEvent* event) override;

private:
  Ui::MainWindowClass ui;

  std::shared_ptr<MediaPlayer> mMediaPlayer;

  // last size and position of the window when not maximized
  // used to restore the window position and size
  Placement mPlacement;
};
