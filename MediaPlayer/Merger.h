#pragma once

#include "Runnable.h"

#include <memory>
#include <fstream>

class Merger : public Runnable
{
public:
  static Ptr create(const QString& videoFilePath
                    , const QString& reversedVideoFilePath
                    , const QString& mergedFilePath
                    , const unsigned loopCount)
  {
    return std::make_unique<Merger>(videoFilePath, reversedVideoFilePath, mergedFilePath, loopCount);
  }

  Merger(const QString& videoFilePath
    , const QString& reversedVideoFilePath
    , const QString& mergedFilePath
    , const unsigned loopCount)
    : Runnable("Merger", "d:/Tools/ffmpeg/ffmpeg.exe", {})
  {
    // create concat list file for ffmpeg process
    mConcatFilePath = getNewFileName("e:/concat.txt");
    {
      std::ofstream ofs(mConcatFilePath.toStdString());
      for (unsigned n = 0; n < loopCount; ++n)
      {
        ofs << "file " << videoFilePath.toStdString() << "\n";
        ofs << "file " << reversedVideoFilePath.toStdString() << "\n";
      }
    }
    // simple concat by copying the files into the output file
    if (true)
    {
      mArguments << "-f" << "concat" << "-safe" << "0" << "-i" << mConcatFilePath << "-c" << "copy" << mergedFilePath << "-y";
    }
    else
    {
      /*
      // try to replace the sound channel by an anrbitary audio file when mering the files with the concat filter (reencoding needed??)
      // fmpeg -f concat -safe 0 -i e:/concat.txt -i e:/audio.mp3 -shortest -c:v copy -c:a copy e:/megan.rain.loves.to.fuck.loop.mp4 -y
      // 
      // 
      //ffmpeg -i video_A.mkv -i video_B.mkv -i audio_C.mp3 \
        -filter_complex '[0:v] [0:a] [1:v] [1:a] [2:a] concat=n=2:v=1:a=2 [v] [a]' \
        -map '[v]' -map '[a]' output.mkv

      //const QString mAudioFilePath("D:\\Zene\\jazzstucks - sample\\SF4997852-02-01-01.mp3");
      //mArguments << "-f " << "concat" <<  "-safe" << "0" "-i" << mConcatFilePath << "-i" << mAudioFilePath;
      //mArguments << "-shortest" << "-c:v" << "copy" << "-c:a" << "copy" << mergedFilePath << "-y";

      //{
      //  std::ofstream ofs("e:\\debug.txt");
      //  for (const auto& arg : mArguments)
      //  {
      //    ofs << arg.toStdString() << " ";
      //  }
      //}
      */
    }
  }

  virtual void onFinished() override
  {
    // mConcatFilePath; todo this runable, and process stuff is not flexible.. a make running a simple function and a process the same
    // interface to simplify usage
  }

private:
  QString mConcatFilePath;
};
