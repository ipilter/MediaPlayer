#pragma once

#include <QUrl>
#include <QString>
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

  std::vector<QUrl> getVideos() const;

  void setOrder(const bool randomize, const bool keepCurrent = false);
  void setCurrentIndex(std::size_t index);
  std::size_t currentIndex() const;
  std::size_t indexOf(const QUrl& url) const;

  QUrl current() const;

  bool next();
  bool previous();

  void setFilter(const QString& pattern);

private:
  void rebuildLookup();

  std::vector<QUrl> mUrls;
  size_t mCurrentIndex = npos;

  std::vector<size_t> mIndices;          // the playback ordering layer (may be shuffled by setOrder).
  std::vector<size_t> mFilteredIndices;  // the filtered view layer (always maintained in original order).

  QHash<QUrl, size_t> mLookup;

  QString mFilterPattern;
};
