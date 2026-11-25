#include "core/state/PruneTracks.h"

#include <algorithm>
#include <optional>
#include <ranges>
#include <set>
#include <string>

#include "core/state/TrackYtmIds.h"

namespace westie::core {

namespace {

std::set<std::string> all_listed(const State& state) {
    std::set<std::string> listed;
    for (const Playlist& playlist : state.playlists | std::views::values) {
        listed.insert(playlist.sources.begin(), playlist.sources.end());
    }
    return listed;
}

bool any_listed(const Track& track, const std::set<std::string>& listed) {
    return std::ranges::any_of(track.sources, [&](const std::string& source) { return listed.contains(source); });
}

}

bool is_listed(const State& state, const Track& track) {
    return any_listed(track, all_listed(state));
}

// The file is wanted again, unless it was made from another id than the track has now
TrackStatus returning_status(const State& state, const Track& track) {
    std::optional<ResolvedYtmId> resolved = resolved_ytm_id(state, track);
    return resolved && resolved->ytm_id == track.ytm_id ? TrackStatus::Downloaded : TrackStatus::Pending;
}

void prune_tracks(State& state) {
    std::set<std::string> listed = all_listed(state);
    for (auto entry = state.tracks.begin(); entry != state.tracks.end();) {
        Track& track = entry->second;
        if (any_listed(track, listed)) {
            if (track.status == TrackStatus::Orphaned || track.status == TrackStatus::Kept) {
                track.status = returning_status(state, track);
            }
            ++entry;
            continue;
        }
        // A file waits for the user to keep or remove it; without one there's nothing to decide on
        if (!track.file.empty()) {
            if (track.status != TrackStatus::Kept) {
                track.status = TrackStatus::Orphaned;
                track.failure = TrackFailure::None;
                track.error.clear();
            }
            ++entry;
            continue;
        }
        for (const std::string& source : track.sources) {
            state.sources.erase(source);
        }
        entry = state.tracks.erase(entry);
    }
}

}
