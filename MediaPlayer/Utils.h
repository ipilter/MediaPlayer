#pragma once

#include <QFile>
#include <QFileInfo>
#include <QString>
#include <QColor>
#include <QRegularExpression>

#include <random>

namespace utils
{

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
inline T Random(T min, T max)
{
  static_assert(std::is_integral<T>::value || std::is_floating_point<T>::value,
                "T must be an integral or floating point type");

  if (min > max) std::swap(min, max);

  thread_local static std::random_device rd;
  thread_local static std::mt19937 gen(rd());

  using CT = typename std::common_type<T>::type;

  if constexpr (std::is_integral<T>::value)
  {
    std::uniform_int_distribution<CT> dis(static_cast<CT>(min), static_cast<CT>(max));  // [a, b] TODO ai said
    return static_cast<T>(dis(gen));
  }
  else
  {
    std::uniform_real_distribution<CT> dis(static_cast<CT>(min), static_cast<CT>(max)); // [a, b) TODO ai said
    return static_cast<T>(dis(gen));
  }
}

QColor inline lerp(const QColor& c1, const QColor& c2, double t)
{
  int r = static_cast<int>(c1.red() + (c2.red() - c1.red()) * t);
  int g = static_cast<int>(c1.green() + (c2.green() - c1.green()) * t);
  int b = static_cast<int>(c1.blue() + (c2.blue() - c1.blue()) * t);
  int a = static_cast<int>(c1.alpha() + (c2.alpha() - c1.alpha()) * t);
  return QColor(r, g, b, a);
}

}