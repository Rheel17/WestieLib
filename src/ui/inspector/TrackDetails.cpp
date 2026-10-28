#include "ui/inspector/TrackDetails.h"

#include <algorithm>
#include <cstddef>
#include <format>
#include <string_view>
#include <utility>

#include "core/sources/SourcePage.h"
#include "core/state/TrackVideos.h"
#include "platform/win32/Utf16.h"
#include "ui/ArtistsText.h"
#include "ui/playlists/PlaylistRow.h"
#include "ui/playlists/PlaylistRows.h"

namespace westie::ui {

namespace {

constexpr int ErrorLines = 2;

std::optional<TrackSource> source_link_of(const core::Track& track, core::SourceType type) {
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

std::optional<TrackMatch> match_of(const core::State& state, const std::string* key) {
    if (key == nullptr) {
        return std::nullopt;
    }
    auto source = state.sources.find(*key);
    if (source == state.sources.end() || !source->second.match) {
        return std::nullopt;
    }
    const core::Match& match = *source->second.match;
    return TrackMatch{
        .video_id = platform::win32::to_utf16(match.video_id),
        .title = platform::win32::to_utf16(match.title),
        .artists = artists_text(match.artists),
        .duration = match.duration,
        .source_duration = source->second.duration,
        .confidence = match.confidence,
    };
}

TrackVideo video(const std::string& id, VideoChooser chosen_by) {
    return {
        .id = platform::win32::to_utf16(id),
        .url = platform::win32::to_utf16(std::format("https://music.youtube.com/watch?v={}", id)),
        .chosen_by = chosen_by,
    };
}

void show_entry_video(const core::State& state, const core::Track& track, const std::string& entry, TrackDetails& details) {
    // How the entry itself got its video, so a duplicate shows what the Matcher did with it
    const std::string* first_spotify = core::first_source_of(track, core::SourceType::Spotify);
    // The user's choice is every entry's
    if (!track.video_override.empty()) {
        details.video = video(track.video_override, VideoChooser::Override);
        details.match = match_of(state, core::source_type_of_key(entry) == core::SourceType::Spotify ? &entry : first_spotify);
        return;
    }
    if (std::optional<std::string> own = core::own_video(entry)) {
        details.video = video(*own, VideoChooser::Playlist);
        details.match = match_of(state, first_spotify);
        return;
    }
    if (auto source = state.sources.find(entry); source != state.sources.end() && source->second.match) {
        details.video = video(source->second.match->video_id, VideoChooser::Matcher);
        details.match = match_of(state, &entry);
        return;
    }
    if (std::optional<core::ResolvedVideo> resolved = core::resolved_video(state, track)) {
        bool chosen_by_playlist = core::source_type_of_key(resolved->source) == core::SourceType::YouTubeMusic;
        details.video = video(resolved->video_id, chosen_by_playlist ? VideoChooser::Playlist : VideoChooser::Matcher);
    }
    details.match = match_of(state, first_spotify);
}

std::wstring last_lines(std::string_view text) {
    while (text.ends_with('\n')) {
        text.remove_suffix(1);
    }
    std::size_t start = text.size();
    for (int line = 0; line < ErrorLines && start != std::string_view::npos; ++line) {
        start = start == 0 ? std::string_view::npos : text.rfind('\n', start - 1);
    }
    return platform::win32::to_utf16(start == std::string_view::npos ? text : text.substr(start + 1));
}

}

std::optional<TrackDetails> track_details(const core::State& state, const TrackRow& row) {
    auto track = state.tracks.find(row.track);
    if (track == state.tracks.end()) {
        return std::nullopt;
    }

    TrackDetails details{.row = row};
    for (core::SourceType type : core::AllSourceTypes) {
        if (std::optional<TrackSource> source = source_link_of(track->second, type)) {
            details.sources.push_back(std::move(*source));
        }
    }
    for (PlaylistRow& playlist : playlist_rows(state)) {
        if (lists_track(state, state.playlists.at(playlist.key), row.track)) {
            details.playlists.push_back({.key = std::move(playlist.key), .name = std::move(playlist.name)});
        }
    }
    show_entry_video(state, track->second, row.key, details);
    if (track->second.status == core::TrackStatus::Failed) {
        details.failure = track->second.failure;
        details.error = last_lines(track->second.error);
    }
    return details;
}

}
