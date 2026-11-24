#include "core/sync/TracksToMatch.h"

#include "core/sources/SourceType.h"
#include "core/state/TrackYtmIds.h"
#include "core/sync/PlaylistOrder.h"

namespace westie::core {

bool needs_match(const State& state, const Track& track) {
    // The user chose an override
    if ((track.status != TrackStatus::Pending && track.status != TrackStatus::Failed) || !track.ytm_override.empty()) {
        return false;
    }
    // The playlist owner chose a YouTube Music source
    if (first_source_of(track, SourceType::YouTubeMusic) != nullptr) {
        return false;
    }
    const std::string* key = first_source_of(track, SourceType::Spotify);
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
