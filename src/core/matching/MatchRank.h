#pragma once

#include <compare>
#include <cstdint>

#include "core/matching/MatchJudgement.h"

namespace westie::core {

struct MatchRank {
    // In the order they count, the higher ranking first
    int verdict = 0;
    int mismatch = 0;
    int fewer_doubts = 0;
    // Before the length, since another artist may be a cover or a reupload, and a length a few seconds off is often
    // the same recording
    int artists = 0;
    int length = 0;
    // The video belongs to a track already, so choosing it keeps the same song from both sources one track
    bool known_video = false;
    bool same_album = false;
    int explicit_content = 0;
    int name = 0;
    // The length difference, negated
    std::int64_t closeness = 0;

    auto operator<=>(const MatchRank&) const = default;
};

MatchRank match_rank(const MatchJudgement& judgement, bool known_video);

}
