#pragma once

#include "Runnable.h"
#include "Types.h"

#include <QString>

#include <memory>

class FastCutter : public Runnable
{
public:
  static Ptr create(const QString& videoPath, const QString cutFilePath, const VTime& startTime, const VTime& endTime);

  FastCutter(const QString& videoPath, const QString cutFilePath, const VTime& startTime, const VTime& endTime);

  virtual void onStarted() override;
  virtual void onFinished() override;
};
