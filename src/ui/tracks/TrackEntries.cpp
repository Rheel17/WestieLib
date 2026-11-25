#include "ui/tracks/TrackEntries.h"

#include <ranges>
#include <set>

#include "ui/playlists/PlaylistRows.h"

namespace westie::ui {

std::vector<TrackEntry> track_entries(const core::State& state) {
    std::vector<TrackEntry> entries;
    std::set<std::string> seen;
    for (const std::string& playlist : playlist_keys(state)) {
        for (const std::string& key : state.playlists.at(playlist).sources) {
            auto source = state.sources.find(key);
            if (source == state.sources.end() || !state.tracks.contains(source->second.track) || !seen.insert(key).second) {
                continue;
            }
            entries.push_back({.source = key, .track = source->second.track});
        }
    }
    // A track that left every playlist still has its sources
    for (const auto& [key, track] : state.tracks) {
        for (const std::string& source : track.sources) {
            if (state.sources.contains(source) && seen.insert(source).second) {
                entries.push_back({.source = source, .track = key});
            }
        }
    }
    return entries;
}

}
