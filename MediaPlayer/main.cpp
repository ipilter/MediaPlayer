#include "MainWindow.h"
#include "MediaPlayer.h"

#include <QtWidgets/QApplication>
#include <QSettings>
#include <QFileInfo>
#include <QUrl>
#include <QMessageBox>
#include <QDir>

#include <fstream>
#include <vector>
#include <random>
#include <algorithm>

void savePreferences(const MainWindow& iMainWindow);
void loadPreferences(MainWindow& iMainWindow);
MediaPlayer::Playlist readPlaylistFile(const QString& iFilePath);
QString readStyles(const QString& iFilePath);

int main(int argc, char *argv[])
{
  QApplication wApp(argc, argv);

  try
  {
    if (argc < 2 || !QFile::exists(argv[1]))
    {
      if (argc > 1)
      {
        throw std::runtime_error("File not found: " + QString(argv[1]).toStdString());
      }
      else
      {
        throw std::runtime_error("Usage: MediaPlayer <playlist.mpl>|<video.mp4>|<directory>");
      }
    }

    {
      const QString wAbsoluteFilePath = QCoreApplication::applicationDirPath() + "/" + "default_style.qss";
      if (!QFile::exists(wAbsoluteFilePath))
      {
        throw std::runtime_error(QString("Cannot open style sheet file: \"%1\"").arg(wAbsoluteFilePath).toStdString());
      }

      const QString wStyles = readStyles(wAbsoluteFilePath);
      wApp.setStyleSheet(wStyles);
    }

    MainWindow wMainWindow;
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

    const QString wInputPath = argv[1];
    {
      const QFileInfo wInputInfo(wInputPath);
      if (wInputInfo.suffix().toLower() == "mpl")
      {
        wMainWindow.setPlaylist(readPlaylistFile(wInputPath));
        wMainWindow.setWindowTitle(QString("Playing playlist: \"%1\"").arg(wInputInfo.completeBaseName()));
      }
      else if (wInputInfo.isDir())
      {
        MainWindow::Playlist playlist;
        for (const auto& wFile : QDir(wInputPath).entryList(QDir::Files))
        {
          playlist.push_back(QUrl::fromLocalFile(wInputPath + "/" + wFile));
        }
        
        std::random_device rd;
        std::mt19937 g(rd());
        std::shuffle(playlist.begin(), playlist.end(), g);

        wMainWindow.setPlaylist(playlist);
        wMainWindow.setWindowTitle(QString("Playing directory: %1").arg(wInputInfo.completeBaseName()));
      }
      else // TODO: validate if know file type...
      {
        MainWindow::Playlist playlist;
        playlist.push_back(QUrl::fromLocalFile(wInputPath));
        wMainWindow.setPlaylist(playlist);
        wMainWindow.setWindowTitle(QString("Playing: %1").arg(wInputInfo.fileName()));
      }
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
void savePreferences(const MainWindow& iMainWindow)
{
  const auto& wPlacement = iMainWindow.getPlacement();

  QSettings settings("IstuSoft", "MediaPlayer");
  settings.beginGroup("MainWindow");
  settings.setValue("size", wPlacement.mSize);
  settings.setValue("pos", wPlacement.mPosition - QPoint(0, 31)); // TODO: what is this??, we move the window in loadPreferences with this value, but when the window gets the event, the y value is bigger than this by 31 pixels!!
  settings.setValue("autoPlay", iMainWindow.getSettings().mAutoPlay);
  settings.setValue("muted", iMainWindow.getSettings().mMuted);
  settings.setValue("firstFrame", iMainWindow.getSettings().mShowFirstFrame);
  settings.endGroup();
}

void loadPreferences(MainWindow& iMainWindow)
{
  QSettings settings("IstuSoft", "MediaPlayer");
  settings.beginGroup("MainWindow");

  auto wSize = settings.value("size", QSize(800, 600)).toSize();
  auto wPosition = settings.value("pos", QPoint(100, 100)).toPoint();

  iMainWindow.resize(wSize);
  iMainWindow.move(wPosition);
  iMainWindow.setSettings(MediaPlayer::Settings{ settings.value("autoPlay", false).toBool(), settings.value("muted", false).toBool(), settings.value("firstFrame", false).toBool() });
  settings.endGroup();
}

MediaPlayer::Playlist readPlaylistFile(const QString& iFilePath)
{
  MediaPlayer::Playlist playlist;
  std::ifstream file(iFilePath.toStdString());
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

QString readStyles(const QString& iFilePath)
{
  QFile file(iFilePath);
  if (!file.open(QFile::ReadOnly | QFile::Text))
  {
    return QString();
  }

  QTextStream stream(&file);
  const QString styleSheet = stream.readAll();
  file.close();
  return styleSheet;
}
