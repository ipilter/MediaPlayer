#include "Filter.h"

#include <QFileInfo>

#include <regex>

namespace details
{

Filter::Filter(Playlist& playlist, const std::string& pattern)
  : mPlaylist(playlist)
  , mPattern(pattern)
{
  // build index based on playlist and the filtered file sublist 
}

}
