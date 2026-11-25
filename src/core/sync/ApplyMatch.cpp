#include "core/sync/ApplyMatch.h"

#include <utility>

#include "core/state/MatchReview.h"
#include "core/state/MergeTracks.h"
#include "core/state/TrackYtmIds.h"

namespace westie::core {

std::optional<std::string> apply_match(State& state, const std::string& track_key, const Match& match) {
    auto track = state.tracks.find(track_key);
    // The user may have chosen a match while this one ran
    if (track == state.tracks.end() || !track->second.ytm_override.empty()) {
        return std::nullopt;
    }
    const std::string* source = match_source_of(state, track->second);
    if (source == nullptr || !state.sources.contains(*source)) {
        return std::nullopt;
    }
    state.sources.at(*source).match = match;
    // A track whose match failed before waits to download again
    if (track->second.status == TrackStatus::Failed) {
        track->second.status = TrackStatus::Pending;
    }
    track->second.failure = TrackFailure::None;
    track->second.error.clear();

    // A match that may be wrong stays apart until it is confirmed
    if (needs_review(match)) {
        return std::nullopt;
    }
    // The track that had the YouTube Music id first survives
    std::optional<std::string> survivor = track_with_ytm_id(state, match.ytm_id, track_key);
    if (survivor) {
        merge_tracks(state, *survivor, track_key);
    }
    return survivor;
}

void apply_match_failure(State& state, const std::string& track_key, TrackFailure failure, std::string error) {
    auto track = state.tracks.find(track_key);
    if (track == state.tracks.end()) {
        return;
    }
    track->second.status = TrackStatus::Failed;
    track->second.failure = failure;
    track->second.error = std::move(error);
}

}
