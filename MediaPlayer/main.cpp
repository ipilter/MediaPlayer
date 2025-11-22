#include "MainWindow.h"
#include "MediaPlayer.h"
#include "Settings.h" // add this include

#include <QtWidgets/QApplication>
#include <QSettings>
#include <QFileInfo>
#include <QUrl>
#include <QMessageBox>
#include <QDir>
#include <QStringList> // for string list conversion

#include <fstream>
#include <algorithm>

std::pair<Playlist, QString> getInputPlaylist(const QString& wInputPath);
void savePreferences(const MainWindow& iMainWindow);
void loadPreferences(MainWindow& iMainWindow);
Playlist readPlaylistFile(const QString& iFilePath);
Playlist readDirectory(const QString& iDirectoryPath);
QString readStyles(const QString& iFilePath);

int main(int argc, char* argv[])
{
  QApplication wApp(argc, argv);
  int wExitStatus = -1;

  try
  {
    // Sanity checks
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

    // Main logic
    {
      // view with controller
      MainWindow wMainWindow;
      loadPreferences(wMainWindow);

      const QString wInputPath = argv[1];
      const auto wPlayList = getInputPlaylist(wInputPath); // playlist, title
      wMainWindow.setPlaylist(wPlayList.first);
      wMainWindow.setWindowTitle(wPlayList.second);

      wMainWindow.show();
      wExitStatus = wApp.exec();

      savePreferences(wMainWindow);
    } // destructor order!
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

  return wExitStatus;
} // main


////////////////
// Implementation of helper functions
void savePreferences(const MainWindow& iMainWindow)
{
  const auto& wPlacement = iMainWindow.getPlacement();

  QSettings settings(QSettings::IniFormat, QSettings::UserScope, "IstuSoft", "MediaPlayer");
  settings.beginGroup("MainWindow");
  settings.setValue("size", wPlacement.mSize);
  settings.setValue("pos", wPlacement.mPosition - QPoint(0, 31)); // TODO: what is this 31 height change in every run??, we move the window in loadPreferences with this value, but when the window gets the event, the y value is bigger than this by 31 pixels!!
  settings.setValue("autoPlay", iMainWindow.getSettings().mAutoPlay);
  settings.setValue("audioMode", static_cast<quint32>(iMainWindow.getSettings().mAudioMode));
  settings.setValue("cursorTimeout", iMainWindow.getSettings().mCursorTimeout);
  settings.setValue("volume", iMainWindow.getSettings().mVolume);
  settings.endGroup();
}

void loadPreferences(MainWindow& iMainWindow)
{
  QSettings settings(QSettings::IniFormat, QSettings::UserScope, "IstuSoft", "MediaPlayer");
  settings.beginGroup("MainWindow");

  auto wSize = settings.value("size", QSize(800, 600)).toSize();
  auto wPosition = settings.value("pos", QPoint(100, 100)).toPoint();

  iMainWindow.resize(wSize);
  iMainWindow.move(wPosition);
  iMainWindow.setSettings(Settings{ settings.value("autoPlay", false).toBool()
                                                 , static_cast<Settings::AudioMode>(settings.value("audioMode", 0).toUInt())
                                                 , settings.value("cursorTimeout", 500).toInt()
                                                 , settings.value("volume", 0.0f).toFloat() });
  settings.endGroup();
}

Playlist readPlaylistFile(const QString& iFilePath)
{
  Playlist wPlaylist;
  std::ifstream wFile(iFilePath.toStdString());
  if (wFile.is_open()) {
    std::string wLine;
    while (std::getline(wFile, wLine)) {
      if (!wLine.empty()) {
        if (wLine.front() == '"' && wLine.back() == '"') {
          wLine = wLine.substr(1, wLine.size() - 2);
        }
        wPlaylist.push_back(QUrl::fromLocalFile(QString::fromStdString(wLine)));
      }
    }
    wFile.close();
  }

  return wPlaylist;
}

Playlist readDirectory(const QString& iDirectoryPath)
{
  Playlist wPlaylist;
  for (const auto& wFile : QDir(iDirectoryPath).entryList(QDir::Files))
  {
    wPlaylist.push_back(QUrl::fromLocalFile(iDirectoryPath + "/" + wFile));
  }

  return wPlaylist;
}

QString readStyles(const QString& iFilePath)
{
  QFile wFile(iFilePath);
  if (!wFile.open(QFile::ReadOnly | QFile::Text))
  {
    return QString();
  }

  QTextStream wStream(&wFile);
  const QString wStyleSheet = wStream.readAll();
  wFile.close();
  return wStyleSheet;
}

std::pair<Playlist, QString> getInputPlaylist(const QString& wInputPath)
{
  const QFileInfo wInputInfo(wInputPath);
  if (wInputInfo.suffix().toLower() == "mpl")
  {
    auto playlist = readPlaylistFile(wInputPath);
    QString title = QString("Playing playlist: \"%1\"").arg(wInputInfo.completeBaseName());
    return { playlist, title };
  }
  else if (wInputInfo.isDir())
  {
    QDir wInputDir(wInputPath);
    auto playlist = readDirectory(wInputPath);
    QString title = QString("Playing directory: %1").arg(
      wInputDir.isRoot() ? wInputPath : wInputInfo.completeBaseName());
    return { playlist, title };
  }
  else // TODO: validate if known file type...
  {
    Playlist playlist;
    playlist.push_back(QUrl::fromLocalFile(wInputPath));
    QString title = QString("Playing: %1").arg(wInputInfo.fileName());
    return { playlist, title };
  }
}
