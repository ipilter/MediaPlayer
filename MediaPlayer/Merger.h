#pragma once

#include "Runnable.h"

class Merger : public Runnable
{
public:
  static Ptr create(const QString& videoFilePath
                    , const QString& reversedVideoFilePath
                    , const QString& mergedFilePath
                    , const unsigned loopCount);

  Merger(const QString& videoFilePath
         , const QString& reversedVideoFilePath
         , const QString& mergedFilePath
         , const unsigned loopCount);

  virtual void onFinished() override;

private:
  QString mConcatFilePath;
};
