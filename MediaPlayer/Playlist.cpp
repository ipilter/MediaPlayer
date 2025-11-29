#include "Playlist.h"
#include "Utils.h"

#include <algorithm>
#include <numeric>
#include <random>
#include <QFileInfo>
#include <QRegularExpression>

Playlist::Playlist(std::vector<QUrl>&& urls)
  : mUrls(std::move(urls))
{
  mFilteredIndices.resize(mUrls.size());
  std::iota(mFilteredIndices.begin(), mFilteredIndices.end(), 0);

  mIndices = mFilteredIndices;

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
  mFilteredIndices.clear();
  mLookup.clear();
  mCurrentIndex = npos;
  mFilterPattern.clear();
}

void Playlist::setOrder(const bool randomize, const bool keepCurrent)
{
  const auto currentUrl = current();

  if (mFilteredIndices.empty())
  {
    mIndices.resize(mUrls.size());
    std::iota(mIndices.begin(), mIndices.end(), 0);
  }
  else
  {
    mIndices = mFilteredIndices;
  }

  if (randomize)
  {
    std::shuffle(mIndices.begin(), mIndices.end(), std::mt19937{ std::random_device{}() });
  }

  rebuildLookup();

  if (keepCurrent)
  {
    setCurrentIndex(indexOf(currentUrl));
  }
  else
  {
    mCurrentIndex = mIndices.empty() ? npos : 0;
  }
}

std::vector<QUrl> Playlist::getVideos() const
{
  std::vector<QUrl> result;
  result.reserve(mFilteredIndices.size());
  for (size_t idx : mFilteredIndices)
  {
    if (idx < mUrls.size())
    {
      result.push_back(mUrls[idx]);
    }
  }
  return result;
}

std::size_t Playlist::currentIndex() const
{
  if (mCurrentIndex == npos || mIndices.empty())
  {
    return npos;
  }
  return mIndices[mCurrentIndex];
}

std::size_t Playlist::indexOf(const QUrl& url) const
{
  if (mUrls.empty() || mIndices.empty())
  {
    return npos;
  }

  auto it = mLookup.find(url);
  if (it == mLookup.end())
  {
    return npos;
  }
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
  if (mUrls.empty() || mIndices.size() <= 1 || mIndices.empty())
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
  if (mUrls.empty() || mIndices.size() <= 1 || mIndices.empty())
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
  {
    return;
  }

  for (size_t pos = 0; pos < mIndices.size(); ++pos)
  {
    const size_t orig = mIndices[pos];
    if (orig < mUrls.size())
    {
      mLookup.insert(mUrls[orig], pos);
    }
  }
}

std::size_t Playlist::viewIndexOfCurrent() const
{
  if (mCurrentIndex == npos || mIndices.empty() || mFilteredIndices.empty())
  {
    return static_cast<std::size_t>(-1);
  }

  const size_t origIndex = mIndices[mCurrentIndex];

  auto it = std::find(mFilteredIndices.begin(), mFilteredIndices.end(), origIndex);
  if (it == mFilteredIndices.end())
  {
    return static_cast<std::size_t>(-1);
  }

  return static_cast<std::size_t>(std::distance(mFilteredIndices.begin(), it));
}

void Playlist::setFilter(const QString& pattern)
{
  mFilterPattern = pattern;
  mFilteredIndices.clear();

  if (mUrls.empty())
  {
    mIndices.clear();
    mLookup.clear();
    mCurrentIndex = npos;
    return;
  }

  size_t currentOrig = npos;
  if (mCurrentIndex != npos && !mIndices.empty())
  {
    currentOrig = mIndices[mCurrentIndex];
  }

  if (mFilterPattern.isEmpty())
  {
    mFilteredIndices.resize(mUrls.size());
    std::iota(mFilteredIndices.begin(), mFilteredIndices.end(), 0);
  }
  else
  {
    QRegularExpression re(mFilterPattern, QRegularExpression::CaseInsensitiveOption);
    const bool valid = re.isValid();

    for (size_t i = 0; i < mUrls.size(); ++i)
    {
      const QString fileName = QFileInfo(mUrls[i].toLocalFile()).completeBaseName();
      bool match = false;
      if (valid)
      {
        match = re.match(fileName).hasMatch();
      }
      else
      {
        match = fileName.contains(mFilterPattern, Qt::CaseInsensitive);
      }

      if (match)
      {
        mFilteredIndices.push_back(i);
      }
    }
  }

  mIndices = mFilteredIndices;

  rebuildLookup();

  if (currentOrig != npos)
  {
    auto it = std::find(mIndices.begin(), mIndices.end(), currentOrig);
    if (it != mIndices.end())
    {
      mCurrentIndex = static_cast<size_t>(std::distance(mIndices.begin(), it));
      return;
    }
  }

  mCurrentIndex = mIndices.empty() ? npos : 0;
}
