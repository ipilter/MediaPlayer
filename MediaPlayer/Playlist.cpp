#include "Playlist.h"

namespace details
{

Playlist::Playlist(const std::vector<QUrl>& urls)
  : mUrls(urls)
{ }

Playlist::Playlist(std::vector<QUrl>&& urls)
  : mUrls(std::move(urls))
{ }

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
}

Playlist::iterator Playlist::begin() { return mUrls.begin(); }
Playlist::iterator Playlist::end() { return mUrls.end(); }
Playlist::const_iterator Playlist::begin() const { return mUrls.begin(); }
Playlist::const_iterator Playlist::end() const { return mUrls.end(); }
Playlist::const_iterator Playlist::cbegin() const { return mUrls.cbegin(); }
Playlist::const_iterator Playlist::cend() const { return mUrls.cend(); }
Playlist::reverse_iterator Playlist::rbegin() { return mUrls.rbegin(); }
Playlist::reverse_iterator Playlist::rend() { return mUrls.rend(); }
Playlist::const_reverse_iterator Playlist::rbegin() const { return mUrls.rbegin(); }
Playlist::const_reverse_iterator Playlist::rend() const { return mUrls.rend(); }

QUrl& Playlist::operator[](std::size_t index)
{
  return mUrls[index];
}

const QUrl& Playlist::operator[](std::size_t index) const
{
  return mUrls[index];
}

} // namespace details
