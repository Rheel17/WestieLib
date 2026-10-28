#include "core/playlists/PlaylistLink.h"

#include <algorithm>
#include <array>
#include <format>
#include <span>

#include "core/sources/LinkUrl.h"

namespace westie::core {

namespace {

constexpr std::string_view YouTubeMusicHost = "music.youtube.com";
constexpr std::string_view BrowsePlaylistPrefix = "VL";
// Liked Music, Watch later and Liked videos belong to one account and are never public
constexpr std::array<std::string_view, 3> PrivateListIds = {"LM", "WL", "LL"};
constexpr std::string_view SpotifyHost = "open.spotify.com";
constexpr std::string_view SpotifyUriPrefix = "spotify:playlist:";
constexpr std::string_view SpotifyRegionPrefix = "intl-";
constexpr std::size_t SpotifyIdLength = 22;

std::optional<PlaylistLink> youtube_playlist(std::string_view id) {
    bool valid = !id.empty() && std::ranges::all_of(id, [](char c) { return is_letter_or_digit(c) || c == '_' || c == '-'; });
    if (!valid || std::ranges::contains(PrivateListIds, id)) {
        return std::nullopt;
    }
    return PlaylistLink{.type = SourceType::YouTubeMusic, .id = std::string(id)};
}

std::optional<PlaylistLink> spotify_playlist(std::string_view id) {
    if (id.size() != SpotifyIdLength || !std::ranges::all_of(id, is_letter_or_digit)) {
        return std::nullopt;
    }
    return PlaylistLink{.type = SourceType::Spotify, .id = std::string(id)};
}

std::optional<PlaylistLink> youtube_link(const LinkUrl& url) {
    if (url.path.size() == 1 && url.path[0] == "playlist") {
        return youtube_playlist(query_value(url.query, "list"));
    }
    if (url.host == YouTubeMusicHost && url.path.size() == 2 && url.path[0] == "browse" && url.path[1].starts_with(BrowsePlaylistPrefix)) {
        return youtube_playlist(url.path[1].substr(BrowsePlaylistPrefix.size()));
    }
    return std::nullopt;
}

std::optional<PlaylistLink> spotify_link(const LinkUrl& url) {
    std::span<const std::string_view> path = url.path;
    if (!path.empty() && path.front().starts_with(SpotifyRegionPrefix)) {
        path = path.subspan(1);
    }
    if (path.size() != 2 || path[0] != "playlist") {
        return std::nullopt;
    }
    return spotify_playlist(path[1]);
}

}

std::string PlaylistLink::key() const {
    return std::format("{}:{}", source_prefix(type), id);
}

std::string PlaylistLink::url() const {
    switch (type) {
    case SourceType::YouTubeMusic: return std::format("https://music.youtube.com/playlist?list={}", id);
    case SourceType::Spotify:      return std::format("https://open.spotify.com/playlist/{}", id);
    }
    return {};
}

std::optional<PlaylistLink> playlist_link_of_key(std::string_view key) {
    std::optional<SourceType> type = source_type_of_key(key);
    if (!type) {
        return std::nullopt;
    }
    return PlaylistLink{.type = *type, .id = std::string(key.substr(source_prefix(*type).size() + 1))};
}

std::optional<PlaylistLink> parse_playlist_link(std::string_view text) {
    text = trim_spaces(text);
    if (starts_with_ignoring_case(text, SpotifyUriPrefix)) {
        return spotify_playlist(text.substr(SpotifyUriPrefix.size()));
    }

    LinkUrl url = split_link_url(text);
    if (std::ranges::contains(YouTubeHosts, url.host)) {
        return youtube_link(url);
    }
    if (url.host == SpotifyHost) {
        return spotify_link(url);
    }
    return std::nullopt;
}

}
