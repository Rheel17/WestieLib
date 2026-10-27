#include "core/sources/SourcePage.h"

#include <format>

#include "core/sources/SourceType.h"

namespace westie::core {

std::optional<std::string> source_page_url(std::string_view key) {
    std::optional<SourceType> type = source_type_of_key(key);
    if (!type) {
        return std::nullopt;
    }
    std::string_view id = key.substr(source_prefix(*type).size() + 1);
    switch (*type) {
    case SourceType::YouTubeMusic: return std::format("https://music.youtube.com/watch?v={}", id);
    case SourceType::Spotify:      return std::format("https://open.spotify.com/track/{}", id);
    }
    return std::nullopt;
}

}
