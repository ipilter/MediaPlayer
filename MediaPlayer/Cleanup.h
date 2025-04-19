#pragma once

#include "Runnable.h"

#include <memory>

class Cleanup : public Runnable
{
public:
  static Ptr create(const QString& filePath)
  {
    return std::make_unique<Cleanup>(filePath);
  }

  Cleanup(const QString& filePath)
    : Runnable("Cleanup", "rm", { filePath })
  {}
};
