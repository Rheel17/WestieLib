#include "core/playlists/PlaylistLink.h"

#include <algorithm>
#include <array>
#include <format>
#include <ranges>
#include <span>
#include <vector>

namespace westie::core {

namespace {

constexpr std::string_view Spaces = " \t\r\n";
constexpr std::array<std::string_view, 2> Schemes = {"https://", "http://"};
constexpr std::array<std::string_view, 4> YouTubeHosts = {"music.youtube.com", "youtube.com", "www.youtube.com", "m.youtube.com"};
constexpr std::string_view YouTubeMusicHost = "music.youtube.com";
constexpr std::string_view BrowsePlaylistPrefix = "VL";
// Liked Music, Watch later and Liked videos belong to one account and are never public
constexpr std::array<std::string_view, 3> PrivateListIds = {"LM", "WL", "LL"};
constexpr std::string_view SpotifyHost = "open.spotify.com";
constexpr std::string_view SpotifyUriPrefix = "spotify:playlist:";
constexpr std::string_view SpotifyRegionPrefix = "intl-";
constexpr std::size_t SpotifyIdLength = 22;

struct Url {
    std::string host{};
    std::vector<std::string_view> path{};
    std::string_view query{};
};

std::string_view trim(std::string_view text) {
    std::size_t first = text.find_first_not_of(Spaces);
    if (first == std::string_view::npos) {
        return {};
    }
    return text.substr(first, text.find_last_not_of(Spaces) - first + 1);
}

char to_lower(char c) noexcept {
    return c >= 'A' && c <= 'Z' ? static_cast<char>(c - 'A' + 'a') : c;
}

bool starts_with_ignoring_case(std::string_view text, std::string_view prefix) {
    return text.size() >= prefix.size() && std::ranges::equal(text.substr(0, prefix.size()), prefix, {}, to_lower, to_lower);
}

bool is_letter_or_digit(char c) noexcept {
    return (c >= '0' && c <= '9') || (c >= 'a' && c <= 'z') || (c >= 'A' && c <= 'Z');
}

std::vector<std::string_view> split(std::string_view text, char separator) {
    std::vector<std::string_view> parts;
    for (auto part : std::views::split(text, separator)) {
        parts.emplace_back(part.begin(), part.end());
    }
    return parts;
}

Url split_url(std::string_view text) {
    for (std::string_view scheme : Schemes) {
        if (starts_with_ignoring_case(text, scheme)) {
            text.remove_prefix(scheme.size());
            break;
        }
    }
    text = text.substr(0, text.find('#'));

    std::size_t host_end = std::min(text.find_first_of("/?"), text.size());
    std::string_view rest = text.substr(host_end);
    std::size_t query_start = std::min(rest.find('?'), rest.size());

    Url url;
    url.host = text.substr(0, host_end) | std::views::transform(to_lower) | std::ranges::to<std::string>();
    // A doubled or trailing slash doesn't change the page
    for (std::string_view name : split(rest.substr(0, query_start), '/')) {
        if (!name.empty()) {
            url.path.push_back(name);
        }
    }
    url.query = rest.substr(std::min(query_start + 1, rest.size()));
    return url;
}

std::string_view query_value(std::string_view query, std::string_view name) {
    for (std::string_view parameter : split(query, '&')) {
        if (parameter.size() > name.size() && parameter.starts_with(name) && parameter[name.size()] == '=') {
            return parameter.substr(name.size() + 1);
        }
    }
    return {};
}

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

std::optional<PlaylistLink> youtube_link(const Url& url) {
    if (url.path.size() == 1 && url.path[0] == "playlist") {
        return youtube_playlist(query_value(url.query, "list"));
    }
    if (url.host == YouTubeMusicHost && url.path.size() == 2 && url.path[0] == "browse" && url.path[1].starts_with(BrowsePlaylistPrefix)) {
        return youtube_playlist(url.path[1].substr(BrowsePlaylistPrefix.size()));
    }
    return std::nullopt;
}

std::optional<PlaylistLink> spotify_link(const Url& url) {
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

std::optional<PlaylistLink> parse_playlist_link(std::string_view text) {
    text = trim(text);
    if (starts_with_ignoring_case(text, SpotifyUriPrefix)) {
        return spotify_playlist(text.substr(SpotifyUriPrefix.size()));
    }

    Url url = split_url(text);
    if (std::ranges::contains(YouTubeHosts, url.host)) {
        return youtube_link(url);
    }
    if (url.host == SpotifyHost) {
        return spotify_link(url);
    }
    return std::nullopt;
}

}
