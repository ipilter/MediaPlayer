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
  // ref c# vcutter.seconds
  //-ss BEGIN -i "SOURCEFILE" -t LENGTH -async 1 -vcodec copy -acodec copy -avoid_negative_ts 1 -y "OUTPUTPATH"

  // preload only if startTime is bigger than 1 second
  VTime preloadTime(1000);
  const VTime duration = endTime - startTime;
  if (startTime >= preloadTime)
  {
    const VTime preloadLength(0, 0, 1); // 1 second
    const VTime preloadTime = startTime - preloadLength;

    // -ss 237  -i "D:\HomeWork\wedge\movies\abby.lee.brazil.all.natural.brazilian.beauty.bitch.mp4" -t 198 -async 1 -vcodec copy -acodec copy -avoid_negative_ts 1 -y "e:\out\abby.lee.brazil.all.natural.brazilian.beauty.bitch.03.57.198.mp4"

    mArguments << "-ss" << preloadTime.toString()
               << "-i" << videoPath
               << "-ss" << preloadLength.toString()
               << "-t" << duration.toString()
               << "-async" << "1"
               << "-vcodec" << "copy"
               << "-acodec" << "copy"
               << "-avoid_negative_ts" << "1"
               << cutFilePath
               << "-y";
  }
  else
  {
    mArguments << "-i" << videoPath
               << "-ss" << startTime.toString()
               << "-t" << duration.toString()
               << "-async" << "1"
               << "-vcodec" << "copy"
               << "-acodec" << "copy"
               << "-avoid_negative_ts" << "1"
               << cutFilePath
               << "-y";
  }
}

void FastCutter::onStarted()
{
  emit logMessage("end");
}
