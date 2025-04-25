#include "MainWindow.h"
#include "MediaPlayer.h"

#include <QtWidgets/QApplication>
#include <QSettings>
#include <QFileInfo>
#include <QUrl>
#include <QMessageBox>

#include <fstream>
#include <vector>
#include <random>
#include <algorithm>

void savePreferences(const MainWindow& window);
void loadPreferences(MainWindow& window);
bool isMediaPlayerPlaylistFile(const QString& filePath);
MediaPlayer::Playlist readFilePaths(const QString& filePath);
QString readStyles(const QString& filePath);

int main(int argc, char *argv[])
{
  QApplication wApp(argc, argv);

  try
  {
    if (argc < 2 || !QFile::exists(argv[1]))
    {
      if (argc > 1)
        throw std::runtime_error("File not found: " + QString(argv[1]).toStdString());
      else
        throw std::runtime_error("Usage: MediaPlayer <playlist.txt>|<video.mp4>");
    }

    const auto filePath = argv[1];

    {
      const QString wAbsoluteFilePath = QCoreApplication::applicationDirPath() + "/" + "default_style.qss";
      QString wStyles = readStyles(wAbsoluteFilePath);
      if (wStyles.isEmpty()) {
        qDebug() << "Failed to read styles from" << wAbsoluteFilePath;
      }
      wApp.setStyleSheet(wStyles);
    }

    MainWindow wMainWindow;
    wMainWindow.setWindowTitle("MediaPlayer v0.0.0");
    loadPreferences(wMainWindow);

    // handle scenario when a multiple files is passed - win open ipc stuff
    // when more than one file is selected windows launches the app for each file !
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

    const int wRet = wApp.exec();
    savePreferences(wMainWindow);

    return wRet;
  }
  catch (const std::exception& e)
  {
    QMessageBox::critical(nullptr, "Error", QString("Error: \"%1\"").arg(e.what()));
    qDebug() << "exception: " << e.what();
    return 1;
  }
  catch (...)
  {
    QMessageBox::critical(nullptr, "Error", QString("Error: unknown"));
    qDebug() << "exception: unknown";
    return 1;
  }

  return 0;
} // main


////////////////
// Implementation of helper functions
void savePreferences(const MainWindow& window)
{
  QSettings settings("IstuSoft", "MediaPlayer");
  settings.beginGroup("MainWindow");
  settings.setValue("size", window.size());
  settings.setValue("pos", window.pos());
  settings.setValue("autoPlay", window.getSettings().mAutoPlay);
  settings.setValue("muted", window.getSettings().mMuted);
  settings.endGroup();
}

void loadPreferences(MainWindow& window)
{
  QSettings settings("IstuSoft", "MediaPlayer");
  settings.beginGroup("MainWindow");
  window.resize(settings.value("size", QSize(800, 600)).toSize());
  window.move(settings.value("pos", QPoint(100, 100)).toPoint());
  window.setSettings(MediaPlayer::Settings{ settings.value("autoPlay", false).toBool(), settings.value("muted", false).toBool() });
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

QString readStyles(const QString& filePath)
{
  QFile file(filePath);
  if (!file.open(QFile::ReadOnly | QFile::Text))
  {
    return QString();
  }

  QTextStream stream(&file);
  const QString styleSheet = stream.readAll();
  file.close();
  return styleSheet;
}
