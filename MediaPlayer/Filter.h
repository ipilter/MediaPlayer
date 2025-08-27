#pragma once

#include "Playlist.h"

namespace details
{

class Filter
{
public:
  explicit Filter(Playlist& playlist, const std::string& pattern);

private:
  Playlist& mPlaylist;
  std::string mPattern;
};

} // namespace details
