#pragma once  

#include <QString>  

class VTime
{
public:
  explicit VTime(const QString& time);
  explicit VTime(qint64 milliseconds = 0);
  VTime(qint64 hour, qint64 minute, qint64 seconds, qint64 milliseconds = 0);
  VTime(const VTime& time);
  ~VTime();

  VTime& operator=(const VTime& time);
  bool operator==(const VTime& time) const;
  bool operator!=(const VTime& time) const;
  bool operator<(const VTime& time) const;
  bool operator<=(const VTime& time) const;
  bool operator>(const VTime& time) const;
  bool operator>=(const VTime& time) const;

  VTime operator - (const VTime& time) const;
  VTime operator + (const VTime& time) const;
  VTime operator * (double factor) const;
  VTime& operator += (const VTime& time);
  VTime& operator -= (const VTime& time);
  VTime& operator *= (double factor);

  qint64 ms() const;
  qint64 seconds() const;

  QString toString(const char sep = ':') const;

  operator QString() const;

private:
  static qint64 getMilliseconds(qint64 hour = 0, qint64 minute = 0, qint64 seconds = 0, qint64 milliseconds = 0);

private:
  qint64 mTime = 0; // milliseconds  
};
