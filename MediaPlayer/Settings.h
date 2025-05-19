#pragma once

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
};
