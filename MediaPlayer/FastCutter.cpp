#include "FastCutter.h"

#include <QString>

#include <memory>

FastCutter::Ptr FastCutter::create(const QString& videoPath, const QString cutFilePath, const VTime& startTime, const VTime& endTime)
{
  return std::make_unique<FastCutter>(videoPath, cutFilePath, startTime, endTime);
}

FastCutter::FastCutter(const QString& videoPath, const QString cutFilePath, const VTime& startTime, const VTime& endTime)
  : Runnable("Cutter", "d:/Tools/ffmpeg/ffmpeg.exe", {})
{
  mArguments << "-ss" << startTime.toString()
             << "-i" << videoPath
             << "-t" << (endTime - startTime).toString()
             << "-async" << "1"
             << "-vcodec" << "copy"
             << "-acodec" << "copy"
             << "-avoid_negative_ts" << "1"
             << cutFilePath
             << "-y";
}

void FastCutter::onStarted()
{
  emit logMessage(QString("Fast cutter started"));
}

void FastCutter::onFinished()
{
  emit logMessage(QString("Fast cutter finished"));
}
