#include "core/sync/PlaylistOrder.h"

#include <algorithm>
#include <cstddef>
#include <map>
#include <optional>
#include <string>
#include <tuple>
#include <utility>

namespace westie::core {

// Playlists by when they were added, then their sources; each track once, with every playlist that lists it
std::vector<QueuedTrack> tracks_in_playlist_order(const State& state, const std::function<bool(const Track& track)>& wanted) {
    std::vector<const std::pair<const std::string, Playlist>*> playlists;
    for (const auto& entry : state.playlists) {
        playlists.push_back(&entry);
    }
    std::ranges::sort(playlists, {}, [](const auto* entry) { return std::tie(entry->second.added_at, entry->first); });

    std::vector<QueuedTrack> tracks;
    // Each track looked at, with its place in tracks when it is wanted
    std::map<std::string, std::optional<std::size_t>> seen;
    for (const auto* playlist : playlists) {
        for (const std::string& key : playlist->second.sources) {
            auto source = state.sources.find(key);
            if (source == state.sources.end()) {
                continue;
            }
            auto [entry, first] = seen.try_emplace(source->second.track);
            if (first) {
                auto track = state.tracks.find(source->second.track);
                if (track != state.tracks.end() && wanted(track->second)) {
                    entry->second = tracks.size();
                    tracks.push_back({.track = track->first});
                }
            }
            if (entry->second) {
                std::vector<std::string>& listed = tracks[*entry->second].playlists;
                if (!std::ranges::contains(listed, playlist->first)) {
                    listed.push_back(playlist->first);
                }
            }
        }
    }
    return tracks;
}

}
