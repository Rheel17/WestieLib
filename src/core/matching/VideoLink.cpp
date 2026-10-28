#include "core/matching/VideoLink.h"

#include <algorithm>

#include "core/sources/LinkUrl.h"

namespace westie::core {

namespace {

constexpr std::string_view ShortHost = "youtu.be";
constexpr std::size_t VideoIdLength = 11;

std::optional<std::string> video_id(std::string_view id) {
    bool valid = id.size() == VideoIdLength && std::ranges::all_of(id, [](char c) { return is_letter_or_digit(c) || c == '_' || c == '-'; });
    return valid ? std::optional(std::string(id)) : std::nullopt;
}

}

std::optional<std::string> parse_video_link(std::string_view text) {
    text = trim_spaces(text);
    if (std::optional<std::string> id = video_id(text)) {
        return id;
    }

    LinkUrl url = split_link_url(text);
    if (url.host == ShortHost && url.path.size() == 1) {
        return video_id(url.path[0]);
    }
    if (!std::ranges::contains(YouTubeHosts, url.host)) {
        return std::nullopt;
    }
    if (url.path.size() == 1 && url.path[0] == "watch") {
        return video_id(query_value(url.query, "v"));
    }
    if (url.path.size() == 2 && url.path[0] == "shorts") {
        return video_id(url.path[1]);
    }
    return std::nullopt;
}

}
