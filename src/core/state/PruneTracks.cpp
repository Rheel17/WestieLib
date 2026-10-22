#include "core/state/PruneTracks.h"

#include <algorithm>
#include <ranges>
#include <set>
#include <string>

namespace westie::core {

namespace {

bool has_file(TrackStatus status) {
    return status == TrackStatus::Downloaded || status == TrackStatus::Orphaned || status == TrackStatus::Kept;
}

}

void prune_tracks(State& state) {
    std::set<std::string> listed;
    for (const Playlist& playlist : state.playlists | std::views::values) {
        listed.insert(playlist.sources.begin(), playlist.sources.end());
    }

    for (auto entry = state.tracks.begin(); entry != state.tracks.end();) {
        Track& track = entry->second;
        if (std::ranges::any_of(track.sources, [&](const std::string& source) { return listed.contains(source); })) {
            ++entry;
            continue;
        }
        // A file waits for the user to keep or remove it; without one there's nothing to decide on
        if (has_file(track.status)) {
            if (track.status == TrackStatus::Downloaded) {
                track.status = TrackStatus::Orphaned;
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
