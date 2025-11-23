#pragma once

#include <QUrl>
#include <QHash>
#include <vector>

class Playlist
{
  constexpr static size_t npos = -1;

public:
  Playlist() = default;
  explicit Playlist(std::vector<QUrl>&& urls);

  std::size_t size() const;
  bool empty() const;
  void clear();

  std::vector<QUrl> GetVideos() const;

  void setOrder(bool randomize);
  void setCurrentIndex(std::size_t index);
  std::size_t currentIndex() const;
  std::size_t indexOf(const QUrl& url) const;

  QUrl current() const;

  bool next();
  bool previous();

private:
  void rebuildLookup();

  std::vector<QUrl> mUrls;
  size_t mCurrentIndex = npos;
  std::vector<size_t> mIndices;
  QHash<QUrl, size_t> mLookup;
};
