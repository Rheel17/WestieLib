#include "core/matching/YouTubeMusicLink.h"

#include <algorithm>

#include "core/sources/LinkUrl.h"

namespace westie::core {

namespace {

constexpr std::string_view ShortHost = "youtu.be";
constexpr std::size_t YtmIdLength = 11;

std::optional<std::string> ytm_id(std::string_view id) {
    bool valid = id.size() == YtmIdLength && std::ranges::all_of(id, [](char c) { return is_letter_or_digit(c) || c == '_' || c == '-'; });
    return valid ? std::optional(std::string(id)) : std::nullopt;
}

}

std::optional<std::string> parse_ytm_link(std::string_view text) {
    text = trim_spaces(text);
    if (std::optional<std::string> id = ytm_id(text)) {
        return id;
    }

    LinkUrl url = split_link_url(text);
    if (url.host == ShortHost && url.path.size() == 1) {
        return ytm_id(url.path[0]);
    }
    if (!std::ranges::contains(YouTubeHosts, url.host)) {
        return std::nullopt;
    }
    if (url.path.size() == 1 && url.path[0] == "watch") {
        return ytm_id(query_value(url.query, "v"));
    }
    if (url.path.size() == 2 && url.path[0] == "shorts") {
        return ytm_id(url.path[1]);
    }
    return std::nullopt;
}

}
