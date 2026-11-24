#include "core/sync/TracksToDownload.h"

#include <optional>

#include "core/state/MatchReview.h"
#include "core/state/TrackYtmIds.h"
#include "core/sync/PlaylistOrder.h"

namespace westie::core {

bool needs_download(const State& state, const Track& track) {
    // A failed match has no id to download; the match is tried again instead
    bool waiting = track.status == TrackStatus::Pending || (track.status == TrackStatus::Failed && track.failure == TrackFailure::Download);
    if (!waiting) {
        return false;
    }
    std::optional<ResolvedYtmId> resolved = resolved_ytm_id(state, track);
    // A match that may be another recording waits for the user
    return resolved && !resolved->ytm_id.empty() && !needs_review(state, track);
}

std::vector<QueuedTrack> tracks_to_download(const State& state) {
    return tracks_in_playlist_order(state, [&](const Track& track) { return needs_download(state, track); });
}

}
