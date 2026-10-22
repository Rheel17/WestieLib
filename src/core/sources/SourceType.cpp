#include "core/sources/SourceType.h"

#include <algorithm>

namespace westie::core {

std::string_view source_prefix(SourceType type) noexcept {
    switch (type) {
    case SourceType::YouTubeMusic: return "ytm";
    case SourceType::Spotify:      return "spotify";
    }
    return {};
}

std::optional<SourceType> source_type_of_key(std::string_view key) noexcept {
    std::string_view prefix = key.substr(0, key.find(':'));
    auto found = std::ranges::find(AllSourceTypes, prefix, source_prefix);
    if (found == AllSourceTypes.end()) {
        return std::nullopt;
    }
    return *found;
}

}
