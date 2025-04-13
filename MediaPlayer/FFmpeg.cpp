#include "FFmpeg.h"

FFmpeg::FFmpeg(const QString& ffmpegPath)
  : mFFmpegPath(ffmpegPath)
{
}


void FFmpeg::runCommand(const QString& command)
{
  // Example: QProcess::start(command);
  return;
}

QString FFmpeg::formatTime(const Time& time) const
{
  return QString();
}

void FFmpeg::cut(const QString& inputFile, const QString& outputFile, const Time startTime, const Time endTime, const bool reconvert)
{
  QString command;
  if (reconvert)
  {
    command = mReconvertCommand.arg(inputFile, formatTime(startTime), formatTime(endTime), outputFile);
  }
  else
  {
    command = mCutCommand.arg(inputFile, formatTime(startTime), formatTime(endTime), outputFile);
  }
  runCommand(command);
}
