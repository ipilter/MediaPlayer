#include "PreciseCutter.h"

#include <QString>

#include <memory>

PreciseCutter::Ptr PreciseCutter::create(const QString& videoPath, const QString cutFilePath, const VTime& startTime, const VTime& endTime)
{
  return std::make_unique<PreciseCutter>(videoPath, cutFilePath, startTime, endTime);
}

PreciseCutter::PreciseCutter(const QString& videoPath, const QString cutFilePath, const VTime& startTime, const VTime& endTime)
  : Runnable("Cutter", "d:/Tools/ffmpeg/ffmpeg.exe", {})
{
  // preload only if startTime is bigger than 1 second
  VTime preloadTime(1000);
  if (startTime >= preloadTime)
  {
    const VTime preloadLength(0, 0, 1); // 1 second
    const VTime preloadTime = startTime - preloadLength;
    const VTime duration = endTime - startTime;

    mArguments << "-ss" << preloadTime.toString()
      << "-i" << videoPath
      << "-ss" << preloadLength.toString()
      << "-t" << duration.toString()
      << "-c:v" << "libx264"
      << "-c:a" << "aac"
      << cutFilePath
      << "-y";
  }
  else
  {
    const VTime duration = endTime - startTime;
    mArguments
      << "-i" << videoPath
      << "-ss" << startTime.toString()
      << "-t" << duration.toString()
      << "-c:v" << "libx264"
      << "-c:a" << "aac"
      << cutFilePath
      << "-y";
  }
}

void PreciseCutter::onStarted()
{
  emit logMessage(QString("Precise cutter started"));
}

void PreciseCutter::onFinished()
{
  emit logMessage(QString("Precise cutter finished"));
}
