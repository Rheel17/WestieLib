#include "core/sync/ApplyFetch.h"

#include <algorithm>
#include <optional>
#include <string>
#include <utility>
#include <vector>

#include "core/state/PruneTracks.h"
#include "core/state/TrackVideos.h"

namespace westie::core {

namespace {

void add_source(State& state, SourceTrack&& listed, std::chrono::system_clock::time_point now) {
    Source& source = state.sources[listed.key];
    source.title = std::move(listed.title);
    source.artists = std::move(listed.artists);
    if (listed.album) {
        source.album = std::move(*listed.album);
    }
    source.duration = listed.duration;
    if (state.tracks.contains(source.track)) {
        return;
    }
    // A YouTube Music video that a Spotify track was matched to is that track's
    if (std::optional<std::string> video = own_video(listed.key)) {
        if (std::optional<std::string> owner = track_with_video(state, *video)) {
            source.track = *owner;
            state.tracks.at(*owner).sources.push_back(listed.key);
            return;
        }
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

void apply_fetch_page(State& state, const std::string& playlist_key, const PlaylistContents& so_far, std::chrono::system_clock::time_point now) {
    auto playlist = state.playlists.find(playlist_key);
    if (playlist == state.playlists.end()) {
        return;
    }

    if (!so_far.name.empty()) {
        playlist->second.name = so_far.name;
    }
    // The earlier sources stay until the whole list is in, so no track leaves the playlist halfway
    std::vector<std::string> earlier = std::exchange(playlist->second.sources, {});
    for (const SourceTrack& listed : so_far.tracks) {
        playlist->second.sources.push_back(listed.key);
        add_source(state, SourceTrack(listed), now);
    }
    for (std::string& key : earlier) {
        if (!std::ranges::contains(playlist->second.sources, key)) {
            playlist->second.sources.push_back(std::move(key));
        }
    }
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
