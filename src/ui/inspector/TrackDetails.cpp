#include "ui/inspector/TrackDetails.h"

#include <algorithm>
#include <utility>

#include "core/sources/SourcePage.h"
#include "platform/win32/Utf16.h"
#include "ui/playlists/PlaylistRow.h"
#include "ui/playlists/PlaylistRows.h"

namespace westie::ui {

namespace {

std::optional<TrackSource> first_source_of(const core::Track& track, core::SourceType type) {
    for (const std::string& key : track.sources) {
        if (core::source_type_of_key(key) != type) {
            continue;
        }
        if (std::optional<std::string> url = core::source_page_url(key)) {
            return TrackSource{.type = type, .url = platform::win32::to_utf16(*url)};
        }
    }
    return std::nullopt;
}

bool lists_track(const core::State& state, const core::Playlist& playlist, const std::string& track) {
    return std::ranges::any_of(playlist.sources, [&](const std::string& key) {
        auto source = state.sources.find(key);
        return source != state.sources.end() && source->second.track == track;
    });
}

}

std::optional<TrackDetails> track_details(const core::State& state, const TrackRow& row) {
    auto track = state.tracks.find(row.key);
    if (track == state.tracks.end()) {
        return std::nullopt;
    }

    TrackDetails details{.row = row};
    for (core::SourceType type : core::AllSourceTypes) {
        if (std::optional<TrackSource> source = first_source_of(track->second, type)) {
            details.sources.push_back(std::move(*source));
        }
    }
    for (PlaylistRow& playlist : playlist_rows(state)) {
        if (lists_track(state, state.playlists.at(playlist.key), row.key)) {
            details.playlists.push_back({.key = std::move(playlist.key), .name = std::move(playlist.name)});
        }
    }
    return details;
}

}
