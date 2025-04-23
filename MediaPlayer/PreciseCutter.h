#pragma once

#include "Runnable.h"
#include "Types.h"

#include <QString>

#include <memory>

class PreciseCutter : public Runnable
{
public:
  static Ptr create(const QString& videoPath, const QString cutFilePath, const VTime& startTime, const VTime& endTime);

  PreciseCutter(const QString& videoPath, const QString cutFilePath, const VTime& startTime, const VTime& endTime);
};
