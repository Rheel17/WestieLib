#include "core/sync/ApplyFetch.h"

#include <algorithm>
#include <utility>

#include "core/state/PruneTracks.h"

namespace westie::core {

namespace {

void add_source(State& state, SourceTrack&& listed, std::chrono::system_clock::time_point now) {
    Source& source = state.sources[listed.key];
    source.title = std::move(listed.title);
    source.artists = std::move(listed.artists);
    source.duration = listed.duration;
    if (state.tracks.contains(source.track)) {
        return;
    }

    // A new source makes a track of its own, keyed like it
    source.track = listed.key;
    auto [entry, created] = state.tracks.try_emplace(listed.key, Track{.added_at = now});
    if (!std::ranges::contains(entry->second.sources, listed.key)) {
        entry->second.sources.push_back(listed.key);
    }
}

}

void apply_fetch(State& state, const std::string& playlist_key, PlaylistContents contents, std::chrono::system_clock::time_point now) {
    auto playlist = state.playlists.find(playlist_key);
    // Removed while it was fetched
    if (playlist == state.playlists.end()) {
        return;
    }

    if (!contents.name.empty()) {
        playlist->second.name = std::move(contents.name);
    }
    playlist->second.sources.clear();
    for (SourceTrack& listed : contents.tracks) {
        playlist->second.sources.push_back(listed.key);
        add_source(state, std::move(listed), now);
    }
    playlist->second.last_sync = now;
    playlist->second.fetch_error.clear();
    playlist->second.fetch_error_at.reset();
    prune_tracks(state);
}

void apply_fetch_failure(State& state, const std::string& playlist_key, std::string error, std::chrono::system_clock::time_point now) {
    auto playlist = state.playlists.find(playlist_key);
    if (playlist == state.playlists.end()) {
        return;
    }
    playlist->second.fetch_error = std::move(error);
    playlist->second.fetch_error_at = now;
}

}
