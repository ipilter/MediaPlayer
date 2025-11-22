#include "Playlist.h"

#include <algorithm>

Playlist::Playlist(const std::vector<QUrl>& urls)
  : mUrls(urls)
{}

Playlist::Playlist(std::vector<QUrl>&& urls)
  : mUrls(std::move(urls))
{}

std::size_t Playlist::size() const
{
  return mUrls.size();
}

bool Playlist::empty() const
{
  return mUrls.empty();
}

void Playlist::push_back(const QUrl& url)
{
  mUrls.push_back(url);
}

void Playlist::push_back(QUrl&& url)
{
  mUrls.push_back(std::move(url));
}

void Playlist::clear()
{
  mUrls.clear();
  mCurrentIndex = 0;
}

const std::vector<QUrl>& Playlist::GetVideos() const
{
  return mUrls;
}


std::size_t Playlist::currentIndex() const
{
  return mCurrentIndex;
}

void Playlist::setCurrentIndex(std::size_t index)
{
  if (mUrls.empty())
  {
    mCurrentIndex = 0;
    return;
  }

  mCurrentIndex = std::min(index, mUrls.size() - 1);
}

QUrl Playlist::current() const
{
  if (mUrls.empty())
  {
    return QUrl();
  }
  return mUrls[mCurrentIndex];
}

bool Playlist::next()
{
  if (mUrls.empty() || mUrls.size() == 1)
  {
    return false;
  }

  if (mCurrentIndex == mUrls.size() - 1)
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
  if (mUrls.empty() || mUrls.size() == 1)
  {
    return false;
  }

  if (mCurrentIndex == 0)
  {
    mCurrentIndex = mUrls.size() - 1;
  }
  else
  {
    --mCurrentIndex;
  }
  return true;
}
