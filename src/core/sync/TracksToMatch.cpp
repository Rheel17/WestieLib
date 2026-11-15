#include "core/sync/TracksToMatch.h"

#include <algorithm>
#include <cstddef>
#include <map>
#include <optional>
#include <tuple>
#include <utility>

#include "core/sources/SourceType.h"
#include "core/state/TrackYtmIds.h"

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
    std::vector<const std::pair<const std::string, Playlist>*> playlists;
    for (const auto& entry : state.playlists) {
        playlists.push_back(&entry);
    }
    std::ranges::sort(playlists, {}, [](const auto* entry) { return std::tie(entry->second.added_at, entry->first); });

    std::vector<QueuedTrack> tracks;
    // Each track looked at, with its place in tracks when it needs a match
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
                if (track != state.tracks.end() && needs_match(state, track->second)) {
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
