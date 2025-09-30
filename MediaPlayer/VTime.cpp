#include "VTime.h"

#include <QString>
#include <QStringList>

VTime::VTime(const QString& time)
{
  QStringList parts = time.split(':');
  if (parts.size() != 3)
  {
    mTime;
  }

  const qint64 h = parts[0].toLongLong();
  const qint64 m = parts[1].toLongLong();
  const qint64 s = parts[2].toLongLong();
  const qint64 ms = parts[3].toLongLong();

  mTime = getMilliseconds(h, m, s, ms);
}

VTime::VTime(qint64 milliseconds)
  : mTime(milliseconds)
{}

VTime::VTime(qint64 hour, qint64 minute, qint64 seconds, qint64 milliseconds)
  : mTime(getMilliseconds(hour, minute, seconds, milliseconds))
{}

VTime::VTime(const VTime& time)
  : mTime(time.mTime)
{}

VTime::~VTime()
{}

VTime& VTime::operator=(const VTime& time)
{
  if (this != &time)
  {
    mTime = time.mTime;
  }
  return *this;
}

bool VTime::operator==(const VTime& time) const
{
  return mTime == time.mTime;
}

bool VTime::operator!=(const VTime& time) const
{
  return mTime != time.mTime;
}

bool VTime::operator<(const VTime& time) const
{
  return mTime < time.mTime;
}

bool VTime::operator<=(const VTime& time) const
{
  return mTime <= time.mTime;
}

bool VTime::operator>(const VTime& time) const
{
  return mTime > time.mTime;
}

bool VTime::operator>=(const VTime& time) const
{
  return mTime >= time.mTime;
}

VTime VTime::operator - (const VTime& time) const
{
  return VTime(mTime - time.mTime);
}

VTime VTime::operator + (const VTime& time) const
{
  return VTime(mTime + time.mTime);
}

VTime VTime::operator * (const double factor) const
{
    return VTime(static_cast<qint64>(mTime * factor));
}

VTime& VTime::operator += (const VTime& time)
{
  mTime += time.mTime;
  return *this;
}

VTime& VTime::operator -= (const VTime& time)
{
  mTime -= time.mTime;
  return *this;
}

VTime& VTime::operator *= (const double factor)
{
  mTime = static_cast<qint64>(mTime * factor);
  return *this;
}

qint64 VTime::ms() const
{
  return mTime;
}

qint64 VTime::seconds() const
{
  return mTime / 1000;
}

QString VTime::toString(const QChar sep) const
{
  const qint64 h = mTime / 3600000;
  const qint64 m = (mTime - h * 3600000) / 60000;
  const qint64 s = (mTime - h * 3600000 - m * 60000) / 1000;
  const qint64 ms = mTime - h * 3600000 - m * 60000 - s * 1000;

  QString ss = QString("%0").arg(h, 2, 10, QChar('0')) + QString(sep) +
    QString("%0").arg(m, 2, 10, QChar('0')) + QString(sep) +
    QString("%0").arg(s, 2, 10, QChar('0')) + QString('.') +
    QString("%0").arg(ms, 3, 10, QChar('0'));
  return ss;
}

qint64 VTime::getMilliseconds(qint64 hour, qint64 minute, qint64 seconds, qint64 milliseconds)
{
  return hour * 3600000 + minute * 60000 + seconds * 1000 + milliseconds;
}

VTime::operator QString() const
{
  return toString();
}
