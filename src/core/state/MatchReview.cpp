#include "core/state/MatchReview.h"

#include <optional>

#include "core/sources/SourceType.h"
#include "core/state/TrackVideos.h"

namespace westie::core {

bool needs_review(const Match& match) {
    return !match.confirmed && match.confidence < ReviewThreshold;
}

bool needs_review(const State& state, const Track& track) {
    std::optional<ResolvedVideo> video = resolved_video(state, track);
    if (!video || video->overridden || source_type_of_key(video->source) != SourceType::Spotify) {
        return false;
    }
    auto source = state.sources.find(video->source);
    return source != state.sources.end() && source->second.match && needs_review(*source->second.match);
}

}
