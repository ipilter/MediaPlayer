#pragma once

#include "Types.h"
#include <QString>

#include <QObject>

class FFmpeg : public QObject
{
  Q_OBJECT

public:
  FFmpeg(const QString& ffmpegPath);

  // non blocking call 
  void cut(const QString& inputFile, const QString& outputFile, const VTime startTime, const VTime endTime, const bool reconvert);

signals:
  void progressUpdated(int percentage);

private:
  // FFmpeg command templates
  const QString mCutCommand = "ffmpeg -i \"%1\" -ss %2 -to %3 -c copy \"%4\"";
  const QString mReconvertCommand = "ffmpeg -i \"%1\" -ss %2 -to %3 -c:v libx264 -crf 23 -preset medium \"%4\"";

  void runCommand(const QString& command);

  // Helper function to format time in HH:MM:SS.milliseconds
  QString formatTime(const VTime& time) const;

  const QString mFFmpegPath;
};
