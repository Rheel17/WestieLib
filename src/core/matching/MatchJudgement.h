#pragma once

#include <vector>

#include "core/matching/MatchEvidence.h"
#include "core/matching/SongFacts.h"
#include "core/state/MatchDoubt.h"

namespace westie::core {

// Ordered from the worst
enum class MatchVerdict { Reject, Review, Accept };
// What a rejected song is: another song, the song by another artist, or another recording of it. Ordered from the
// furthest off
enum class MatchMismatch { Song, Artist, Recording, None };

struct MatchJudgement {
    MatchVerdict verdict = MatchVerdict::Reject;
    MatchMismatch mismatch = MatchMismatch::Song;
    std::vector<MatchDoubt> doubts{};
    MatchEvidence evidence{};
    // From 0 to 1: at least the review threshold when accepted, under it otherwise
    double confidence = 0.0;
};

MatchJudgement judge_match(const SongFacts& wanted, const SongFacts& found);

}
