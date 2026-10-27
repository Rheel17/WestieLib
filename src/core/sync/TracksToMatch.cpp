#include "core/sync/TracksToMatch.h"

#include <algorithm>
#include <set>
#include <tuple>
#include <utility>

#include "core/sources/SourceType.h"
#include "core/state/TrackVideos.h"

namespace westie::core {

bool needs_match(const State& state, const Track& track) {
    if (track.status != TrackStatus::Pending && track.status != TrackStatus::Failed) {
        return false;
    }
    // The playlist owner chose a YouTube Music source's video
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

std::vector<std::string> tracks_to_match(const State& state) {
    std::vector<const std::pair<const std::string, Playlist>*> playlists;
    for (const auto& entry : state.playlists) {
        playlists.push_back(&entry);
    }
    std::ranges::sort(playlists, {}, [](const auto* entry) { return std::tie(entry->second.added_at, entry->first); });

    std::vector<std::string> tracks;
    std::set<std::string> seen;
    for (const auto* playlist : playlists) {
        for (const std::string& key : playlist->second.sources) {
            auto source = state.sources.find(key);
            if (source == state.sources.end() || !seen.insert(source->second.track).second) {
                continue;
            }
            auto track = state.tracks.find(source->second.track);
            if (track != state.tracks.end() && needs_match(state, track->second)) {
                tracks.push_back(track->first);
            }
        }
    }
    return tracks;
}

}
