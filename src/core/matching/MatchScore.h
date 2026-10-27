#pragma once

#include <chrono>
#include <string>
#include <string_view>
#include <vector>

#include "core/matching/MatchText.h"
#include "platform/TextNormalizer.h"

namespace westie::core {

struct ComparableSong {
    MatchText title{};
    std::vector<MatchText> artists{};
    std::chrono::milliseconds duration{};
};

ComparableSong comparable_song(
        const platform::TextNormalizer& normalizer, std::string_view title, const std::vector<std::string>& artists, std::chrono::milliseconds duration
);
double match_score(const ComparableSong& wanted, const ComparableSong& found);

}
