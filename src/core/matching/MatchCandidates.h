#pragma once

#include <chrono>
#include <string>
#include <vector>

#include "core/matching/MatchJudgement.h"
#include "core/matching/YouTubeMusicSearch.h"
#include "core/state/MatchDoubt.h"
#include "core/state/State.h"
#include "platform/TextNormalizer.h"

namespace westie::core {

struct MatchCandidate {
    std::string ytm_id{};
    std::string title{};
    std::vector<std::string> artists{};
    std::string album{};
    std::chrono::milliseconds duration{};
    // The Matcher's verdict on it for the source
    MatchVerdict verdict = MatchVerdict::Reject;
    MatchMismatch mismatch = MatchMismatch::Song;
    std::vector<MatchDoubt> doubts{};
    // Whether the source's track has this YouTube Music id
    bool current = false;
    // Another track that has this YouTube Music id, which choosing it merges into
    std::string track{};
};

// For a Spotify source: its match, the library's YouTube Music sources the Matcher doesn't reject, and the search
// results, each YouTube Music id once, the best first
std::vector<MatchCandidate> match_candidates(
        const State& state, const platform::TextNormalizer& normalizer, const std::string& source_key, const std::vector<SongResult>& results
);

}
