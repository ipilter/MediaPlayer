#pragma once

#include "Runnable.h"

#include <memory>

class Reverser : public Runnable
{
public:
  static Ptr create(const QString& originalFilePath, const QString& reversedFilePath)
  {
    return std::make_unique<Reverser>(originalFilePath, reversedFilePath);
  }

  Reverser(const QString& originalFilePath, const QString& reversedFilePath)
    : Runnable("Reverser", "d:/Tools/ffmpeg/ffmpeg.exe", {})
  {
    mArguments << "-i" << originalFilePath << "-vf" << "reverse" << reversedFilePath << "-y";
  }
};
