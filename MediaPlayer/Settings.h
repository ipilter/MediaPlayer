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
  int mCursorTimeout = 1000;
  float mVolume = 0.0f;
  bool mRandomize = false;
};
