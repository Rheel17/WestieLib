#include "core/sync/TracksToMatch.h"

#include "core/state/TrackYtmIds.h"
#include "core/sync/PlaylistOrder.h"

namespace westie::core {

bool needs_match(const State& state, const Track& track) {
    if (track.status != TrackStatus::Pending && track.status != TrackStatus::Failed) {
        return false;
    }
    // None for an override, or a YouTube Music source the playlist owner chose that YouTube Music still plays
    const std::string* key = source_to_match(state, track);
    if (key == nullptr) {
        return false;
    }
    auto source = state.sources.find(*key);
    return source != state.sources.end() && !source->second.match;
}

std::vector<QueuedTrack> tracks_to_match(const State& state) {
    return tracks_in_playlist_order(state, [&](const Track& track) { return needs_match(state, track); });
}

}
