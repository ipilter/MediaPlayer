#include "Reverser.h"

#include <memory>

Reverser::Ptr Reverser::create(const QString& originalFilePath, const QString& reversedFilePath)
{
  return std::make_unique<Reverser>(originalFilePath, reversedFilePath);
}

Reverser::Reverser(const QString& originalFilePath, const QString& reversedFilePath)
 : Runnable("Reverser", "d:/Tools/ffmpeg/ffmpeg.exe", {})
{
  mArguments << "-i" << originalFilePath << "-vf" << "reverse" << reversedFilePath << "-y";
}
