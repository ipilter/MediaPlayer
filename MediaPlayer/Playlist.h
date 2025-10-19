#pragma once

#include <vector>
#include <string>

#include <QUrl>

namespace details
{
class Playlist
{
public:
  explicit Playlist(const std::vector<QUrl>& urls = {});
  explicit Playlist(std::vector<QUrl>&& urls);

  using iterator = std::vector<QUrl>::iterator;
  using const_iterator = std::vector<QUrl>::const_iterator;
  using reverse_iterator = std::vector<QUrl>::reverse_iterator;
  using const_reverse_iterator = std::vector<QUrl>::const_reverse_iterator;

  std::size_t size() const;
  bool empty() const;
  void push_back(const QUrl& url);
  void push_back(QUrl&& url);
  void clear();

  iterator begin();
  iterator end();
  const_iterator begin() const;
  const_iterator end() const;
  const_iterator cbegin() const;
  const_iterator cend() const;
  reverse_iterator rbegin();
  reverse_iterator rend();
  const_reverse_iterator rbegin() const;
  const_reverse_iterator rend() const;

  QUrl& operator[](std::size_t index);
  const QUrl& operator[](std::size_t index) const;

  const std::vector<QUrl>& GetVideos() const;

private:
  std::vector<QUrl> mUrls;
};

} // namespace details
