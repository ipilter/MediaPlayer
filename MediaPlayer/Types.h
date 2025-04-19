#pragma once

#include <QtCore/qglobal.h>
#include <QString>
#include <QStringList>  // TODO move to cpp

#include <vector>

class VTime
{
public:
  VTime(const QString& time)
  {
    QStringList parts = time.split(':');
    if (parts.size() != 3)
    {
      mTime = 0;
    }

    const qint64 h = parts[0].toLongLong();
    const qint64 m = parts[1].toLongLong();
    const qint64 s = parts[2].toLongLong();
    const qint64 ms = parts[3].toLongLong();

    mTime = getMilliseconds(h, m, s, ms);
  }

  VTime(qint64 milliseconds = 0)
    : mTime(milliseconds)
  {}

  VTime(qint64 hour, qint64 minute, qint64 seconds, qint64 milliseconds = 0)
    : mTime(getMilliseconds(hour, minute, seconds, milliseconds))
  {}

  VTime(const VTime& time)
    : mTime(time.mTime)
  {}

  ~VTime()
  {}

  VTime& operator=(const VTime& time)
  {
    if (this != &time)
    {
      mTime = time.mTime;
    }
    return *this;
  }

  bool operator==(const VTime& time) const
  {
    return mTime == time.mTime;
  }

  bool operator!=(const VTime& time) const
  {
    return mTime != time.mTime;
  }

  bool operator<(const VTime& time) const
  {
    return mTime < time.mTime;
  }

  bool operator<=(const VTime& time) const
  {
    return mTime <= time.mTime;
  }

  bool operator>(const VTime& time) const
  {
    return mTime > time.mTime;
  }

  bool operator>=(const VTime& time) const
  {
    return mTime >= time.mTime;
  }

  QString toString(const QChar sep = ':') const
  {
    const qint64 h = mTime / 3600000;
    const qint64 m = (mTime - h * 3600000) / 60000;
    const qint64 s = (mTime - h * 3600000 - m * 60000) / 1000;
    const qint64 ms = mTime - h * 3600000 - m * 60000 - s * 1000;

    //ugly + below as:
    //QString s1 = QString("%1%2%3").arg("0").arg("1").arg('2');  // 012
    //QString s2 = QString("%1%3%2").arg("0").arg("1").arg('2');  // 02  !! bug or limitation?

    QString ss = QString("%0").arg(h, 2, 10, QChar('0')) + QString(sep) +
      QString("%0").arg(m, 2, 10, QChar('0')) + QString(sep) +
      QString("%0").arg(s, 2, 10, QChar('0')) + QString('.') +
      QString("%0").arg(ms, 3, 10, QChar('0'));
    return ss;
  }

  VTime operator - (const VTime& time) const
  {
    return VTime(mTime - time.mTime);
  }

  VTime operator + (const VTime& time) const
  {
    return VTime(mTime + time.mTime);
  }

  VTime& operator += (const VTime& time)
  {
    mTime += time.mTime;
    return *this;
  }

  VTime& operator -= (const VTime& time)
  {
    mTime -= time.mTime;
    return *this;
  }

  qint64 ms() const
  {
    return mTime;
  }

  qint64 seconds() const
  {
    return mTime / 1000;
  }

private:
  static qint64 getMilliseconds(qint64 hour = 0, qint64 minute = 0, qint64 seconds = 0, qint64 milliseconds = 0)
  {
    return hour * 3600000 + minute * 60000 + seconds * 1000 + milliseconds;
  }

private:
  qint64 mTime = 0; // milliseconds
};



using Sequence = std::pair<VTime, VTime>;
using Sequences = std::vector<Sequence>;