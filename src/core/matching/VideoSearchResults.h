#pragma once

#include <string>
#include <vector>

#include "core/matching/VideoCandidates.h"

namespace westie::core {

struct VideoSearchResults {
    // The number VideoSearch::search returned for it
    int request = 0;
    std::vector<VideoCandidate> candidates{};
    // Why the search failed; the candidates then have no search results
    std::string error{};
};

}
