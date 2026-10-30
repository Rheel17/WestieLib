#pragma once

#include <chrono>
#include <optional>
#include <string>
#include <string_view>
#include <vector>

#include "core/matching/TitleParts.h"
#include "platform/TextNormalizer.h"

namespace westie::core {

struct SongFacts {
    TitleParts title{};
    std::vector<std::vector<std::string>> artists{};
    // The names in an artist entry that holds several, as Kina Grannis in "Kina Grannis, Kurt Hugo Schneider"
    std::vector<std::vector<std::string>> artist_pieces{};
    // The album's whole name, without its parts, as "Night Visions" for "Night Visions (Deluxe)"
    std::vector<std::string> album{};
    std::chrono::milliseconds duration{};
    std::optional<bool> explicit_content{};
};

SongFacts song_facts(
        const platform::TextNormalizer& normalizer, std::string_view title, const std::vector<std::string>& artists, std::string_view album,
        std::chrono::milliseconds duration, std::optional<bool> explicit_content = std::nullopt
);

}
