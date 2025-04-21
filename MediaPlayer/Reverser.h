#pragma once

#include "Runnable.h"

class Reverser : public Runnable
{
public:
  static Ptr create(const QString& originalFilePath, const QString& reversedFilePath);

  Reverser(const QString& originalFilePath, const QString& reversedFilePath);

  virtual void onStarted() override;
  virtual void onFinished() override;
};
