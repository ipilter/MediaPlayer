#pragma once

#include <QUrl>

#include <vector>
#include <string>

class Playlist
{
public:
  explicit Playlist(const std::vector<QUrl>& urls = {});
  explicit Playlist(std::vector<QUrl>&& urls);

  std::size_t size() const;
  bool empty() const;
  void push_back(const QUrl& url);
  void push_back(QUrl&& url);
  void clear();

  const std::vector<QUrl>& GetVideos() const;

  std::size_t currentIndex() const;
  void setCurrentIndex(std::size_t index);
  QUrl current() const;
  bool next(); 
  bool previous();

private:
  std::vector<QUrl> mUrls;
  std::size_t mCurrentIndex = 0;
};
