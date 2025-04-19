#pragma once

#include "Runnable.h"
#include "MainWindow.h"

#include <QString>

#include <memory>

class Cutter : public Runnable
{
public:
  static Ptr create(const QString& videoPath, const QString cutFilePath
                    , const VTime& startTime, const VTime& endTime)
  {
    return std::make_unique<Cutter>(videoPath, cutFilePath, startTime, endTime);
  }

  Cutter(const QString& videoPath, const QString cutFilePath
         , const VTime& startTime, const VTime& endTime)
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

  void onStarted() override
  {
    emit logMessage("end");
  }
};