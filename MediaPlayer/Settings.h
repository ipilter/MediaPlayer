#pragma once

#include <vector>
#include <string>
#include "CacheData.h"

struct Settings
{
  enum class AudioMode : unsigned
  {
    Muted = 0,
    Video,
    Music,
    Last
  };

  bool mAutoPlay = false;
  AudioMode mAudioMode = AudioMode::Muted;
  int mCursorTimeout = 1000;
  std::vector<std::string> mRawFolders;
  CacheData mCacheData;
};
