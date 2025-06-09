#pragma once

#include <vector>
#include <string>

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
  bool mShowFirstFrame = false;
  int mCursorTimeout = 1000;
  std::vector<std::string> mRawFolders;
};
