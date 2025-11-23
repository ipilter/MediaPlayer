#include "Playlist.h"
#include "Utils.h"

#include <algorithm>
#include <numeric>

Playlist::Playlist(std::vector<QUrl>&& urls)
  : mUrls(std::move(urls))
{
  mIndices.resize(mUrls.size());
  std::iota(mIndices.begin(), mIndices.end(), 0);
  mCurrentIndex = mUrls.empty() ? npos : 0;
  rebuildLookup();
}

std::size_t Playlist::size() const
{
  return mUrls.size();
}

bool Playlist::empty() const
{
  return mUrls.empty();
}

void Playlist::clear()
{
  mUrls.clear();
  mIndices.clear();
  mLookup.clear();
  mCurrentIndex = npos;
}

void Playlist::setOrder(const bool randomize, const bool keepCurrent)
{
  const auto currentUrl = current(); // store current URL to restore current video after shuffling
  mIndices.resize(mUrls.size());
  std::iota(mIndices.begin(), mIndices.end(), 0);
  if(randomize)
  {
    std::shuffle(mIndices.begin(), mIndices.end(), std::mt19937{ std::random_device{}() });
  }
  rebuildLookup();
 
  if (keepCurrent)
  {
    setCurrentIndex(indexOf(currentUrl));
  }
}

std::vector<QUrl> Playlist::GetVideos() const
{
  return mUrls;
}

std::size_t Playlist::currentIndex() const
{
  if (mCurrentIndex == npos || mIndices.empty())
    return npos;

  return mIndices[mCurrentIndex];
}

std::size_t Playlist::indexOf(const QUrl& url) const
{
  if (mUrls.empty() || mIndices.empty())
    return npos;

  auto it = mLookup.find(url);
  if (it == mLookup.end())
    return npos;

  return static_cast<size_t>(*it);
}

void Playlist::setCurrentIndex(std::size_t index)
{
  if (mUrls.empty() || mIndices.empty())
  {
    mCurrentIndex = npos;
    return;
  }
  mCurrentIndex = index;
}

QUrl Playlist::current() const
{
  if (mUrls.empty() || mCurrentIndex == npos || mIndices.empty())
  {
    return QUrl();
  }
  return mUrls[currentIndex()];
}

bool Playlist::next()
{
  if (mUrls.empty() || mUrls.size() == 1 || mIndices.empty())
  {
    return false;
  }

  if (mCurrentIndex == mIndices.size() - 1)
  {
    mCurrentIndex = 0;
  }
  else
  {
    ++mCurrentIndex;
  }
  return true;
}

bool Playlist::previous()
{
  if (mUrls.empty() || mUrls.size() == 1 || mIndices.empty())
  {
    return false;
  }

  if (mCurrentIndex == 0)
  {
    mCurrentIndex = mIndices.size() - 1;
  }
  else
  {
    --mCurrentIndex;
  }
  return true;
}

void Playlist::rebuildLookup()
{
  mLookup.clear();
  if (mIndices.empty() || mUrls.empty())
    return;

  for (size_t pos = 0; pos < mIndices.size(); ++pos)
  {
    const size_t orig = mIndices[pos];
    if (orig < mUrls.size())
      mLookup.insert(mUrls[orig], pos);
  }
}
