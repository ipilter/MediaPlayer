#pragma once

#include <QFile>
#include <QFileInfo>
#include <QString>
#include <QRegularExpression>

#include <random>

inline QString prettifyFileName(QString fileName)
{
  fileName.replace(QRegularExpression("[^A-Za-z]"), ".");
  fileName.replace(QRegularExpression("\\.{2,}"), ".");
  return fileName.toLower();
}

inline QString uniqueFileName(const QString& fileName)
{
  size_t wIdx = 0;
  QString wFileName = fileName;
  while (QFile::exists(wFileName))
  {
    wFileName = fileName + QString(".%1").arg(++wIdx);
  }

  return wFileName;
}

template <typename T>
inline T Random(const T min, const T max)
{
  static_assert(std::is_integral<T>::value || std::is_floating_point<T>::value, "T must be an integral or floating point type");

  static std::random_device rd;
  std::mt19937 gen(rd());
  std::uniform_int_distribution<> dis(min, max);
  return static_cast<T>(dis(gen));
};
