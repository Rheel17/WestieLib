#pragma once

#include <string>
#include <vector>

#include "core/matching/MatchCandidates.h"

namespace westie::core {

struct CandidateSearchResults {
    // The number CandidateSearch::search returned for it
    int request = 0;
    std::vector<MatchCandidate> candidates{};
    // Why the search failed; the candidates then have no search results
    std::string error{};
};

}
