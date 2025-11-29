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
  // initialize filtered indices to full set (no filter)
  mFilteredIndices.resize(mUrls.size());
  std::iota(mFilteredIndices.begin(), mFilteredIndices.end(), 0);

  // playback indices default to filtered (which is the full set here)
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
  // store current URL to restore current video after reordering if requested
  const auto currentUrl = current();

  // base ordering is the filtered set (so playback operates on the filtered subset)
  if (mFilteredIndices.empty())
  {
    // if no filtered indices (shouldn't happen except empty playlist), fall back to full range
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
    // reset playback pointer to start of the playback indices
    mCurrentIndex = mIndices.empty() ? npos : 0;
  }
}

std::vector<QUrl> Playlist::getVideos() const
{
  // Return the filtered list (in original order), unaffected by playback ordering (mIndices).
  std::vector<QUrl> result;
  result.reserve(mFilteredIndices.size());
  for (size_t idx : mFilteredIndices)
  {
    if (idx < mUrls.size())
      result.push_back(mUrls[idx]);
  }
  return result;
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
    return;

  for (size_t pos = 0; pos < mIndices.size(); ++pos)
  {
    const size_t orig = mIndices[pos];
    if (orig < mUrls.size())
      mLookup.insert(mUrls[orig], pos);
  }
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

  if (mFilterPattern.isEmpty())
  {
    mFilteredIndices.resize(mUrls.size());
    std::iota(mFilteredIndices.begin(), mFilteredIndices.end(), 0);
  }
  else
  {
    QRegularExpression re(mFilterPattern, QRegularExpression::CaseInsensitiveOption);
    const bool valid = re.isValid();
    if (!valid)
    {
      return;
    }

    for (size_t i = 0; i < mUrls.size(); ++i)
    {
      const QString fileName = QFileInfo(mUrls[i].toLocalFile()).completeBaseName();
      bool match = false;
      match = re.match(fileName).hasMatch();
      if (match)
        mFilteredIndices.push_back(i);
    }
  }

  mIndices = mFilteredIndices;
  mCurrentIndex = mIndices.empty() ? npos : 0;

  rebuildLookup();
}
