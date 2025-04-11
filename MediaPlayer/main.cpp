#include "MainWindow.h"

#include "MediaPlayer.h"

#include <QString>
#include <QtWidgets/QApplication>
#include <QSettings>
#include <QFileInfo>

#include <fstream>
#include <vector>
#include <random>
#include <algorithm>

void savePreferences(const MainWindow& window);
void loadPreferences(MainWindow& window);
bool isMediaPlayerPlaylistFile(const QString& filePath);
MediaPlayer::Playlist readFilePaths(const QString& filePath);

int main(int argc, char *argv[])
{
  QApplication a(argc, argv);
  
  MainWindow wMainWindow;
  wMainWindow.setWindowTitle("MediaPlayer v0.0.0");
  loadPreferences(wMainWindow);

  if (argc < 1) {
    qDebug() << "Usage: MediaPlayer <playlist.txt>|<video.mp4>";
    return 1;
  }

  const auto filePath = argv[1];
  if (!QFile::exists(filePath)) {
    qDebug() << "File does not exist.";
    return 1;
  }

  // handle scenario when a multiple files is passed - win open ipc stuff
  // how windows handles when I issue Open on multiple selected files, seems a first selected file is passed !! 
  // 
  // 
  //std::ofstream outputFile("e:/args.txt");
  //if (outputFile.is_open()) {
  //  for (int i = 0; i < argc; i++) {
  //    outputFile << argv[i] << std::endl;
  //  }
  //  outputFile.close();
  //} else {
  //  qDebug() << "Failed to open output file.";
  //}
  //return 1;

  if (isMediaPlayerPlaylistFile(filePath))
  {
     wMainWindow.setPlaylist(readFilePaths(filePath));
  }
  else
  {
    MainWindow::Playlist playlist;
    playlist.push_back(QUrl::fromLocalFile(filePath));
    wMainWindow.setPlaylist(playlist);
  }

  wMainWindow.show();

  const int ret = a.exec();  
  savePreferences(wMainWindow);
  return ret;
}

void savePreferences(const MainWindow& window)
{
  QSettings settings("IstuSoft", "MediaPlayer");
  settings.beginGroup("MainWindow");
  settings.setValue("size", window.size());
  settings.setValue("pos", window.pos());
  settings.setValue("autoPlay", window.getSettings().mAutoPlay);
  settings.endGroup();
}

void loadPreferences(MainWindow& window)
{
  QSettings settings("IstuSoft", "MediaPlayer");
  settings.beginGroup("MainWindow");
  window.resize(settings.value("size", QSize(800, 600)).toSize());
  window.move(settings.value("pos", QPoint(100, 100)).toPoint());
  window.setSettings(MediaPlayer::Settings{ settings.value("autoPlay", false).toBool() });
  settings.endGroup();
}

bool isMediaPlayerPlaylistFile(const QString& filePath)
{
  return QFileInfo(filePath).suffix().toLower() == "mpl";
}

MediaPlayer::Playlist readFilePaths(const QString& filePath)
{
  MediaPlayer::Playlist playlist;
  std::ifstream file(filePath.toStdString());
  if (file.is_open()) {
    std::string line;
    while (std::getline(file, line)) {
      if (!line.empty()) {
        if (line.front() == '"' && line.back() == '"') {
          line = line.substr(1, line.size() - 2);
        }
        playlist.push_back(QUrl::fromLocalFile(QString::fromStdString(line)));
      }
    }
    file.close();
  }
  
  std::random_device rd;
  std::mt19937 g(rd());
  std::shuffle(playlist.begin(), playlist.end(), g);

  return playlist;
}
