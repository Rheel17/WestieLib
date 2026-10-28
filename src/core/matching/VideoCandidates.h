#pragma once

#include <chrono>
#include <string>
#include <vector>

#include "core/matching/YouTubeMusicSearch.h"
#include "core/state/State.h"
#include "platform/TextNormalizer.h"

namespace westie::core {

struct VideoCandidate {
    std::string video_id{};
    std::string title{};
    std::vector<std::string> artists{};
    std::string album{};
    std::chrono::milliseconds duration{};
    // How well it matches the source, from 0 to 1
    double score = 0.0;
    // The video the source's track has
    bool current = false;
    // Another track that has the video, which choosing it merges into
    std::string track{};
};

// For a Spotify source: its match, the library's YouTube Music sources that come close, and the search results, each
// video once, the best first
std::vector<VideoCandidate> video_candidates(
        const State& state, const platform::TextNormalizer& normalizer, const std::string& source_key, const std::vector<SongResult>& results
);

}
