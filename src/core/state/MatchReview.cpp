#include "core/state/MatchReview.h"

#include <optional>

#include "core/sources/SourceType.h"
#include "core/state/TrackYtmIds.h"

namespace westie::core {

bool needs_review(const Match& match) {
    return !match.confirmed && match.confidence < ReviewThreshold;
}

bool needs_review(const State& state, const Track& track) {
    std::optional<ResolvedYtmId> resolved = resolved_ytm_id(state, track);
    if (!resolved || resolved->overridden || source_type_of_key(resolved->source) != SourceType::Spotify) {
        return false;
    }
    auto source = state.sources.find(resolved->source);
    return source != state.sources.end() && source->second.match && needs_review(*source->second.match);
}

}
