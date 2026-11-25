#include "ui/inspector/TrackDetails.h"

#include <algorithm>
#include <cstddef>
#include <format>
#include <string_view>
#include <utility>

#include "core/matching/MatchEvidence.h"
#include "core/sources/SourcePage.h"
#include "core/state/EffectiveMetadata.h"
#include "core/state/TrackYtmIds.h"
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

std::wstring words_text(const std::vector<std::string>& words) {
    std::wstring text;
    for (const std::string& word : words) {
        text += (text.empty() ? L"" : L" ") + platform::win32::to_utf16(word);
    }
    return text;
}

std::optional<TrackMatch> match_of(const core::State& state, const platform::TextNormalizer& normalizer, const std::string* key) {
    if (key == nullptr) {
        return std::nullopt;
    }
    auto source = state.sources.find(*key);
    if (source == state.sources.end() || !source->second.match) {
        return std::nullopt;
    }
    const core::Match& match = *source->second.match;
    core::VersionDifference difference = core::version_difference(normalizer, source->second.title, source->second.artists, match.title, match.artists);
    return TrackMatch{
        .ytm_id = platform::win32::to_utf16(match.ytm_id),
        .title = platform::win32::to_utf16(match.title),
        .artists = artists_text(match.artists),
        .duration = match.duration,
        .source_duration = source->second.duration,
        .confidence = match.confidence,
        .doubts = match.doubts,
        .explicit_content = match.explicit_content,
        .source_words = words_text(difference.wanted),
        .result_words = words_text(difference.found),
    };
}

TrackAudio audio(const std::string& id, AudioChooser chosen_by) {
    return {
        .id = platform::win32::to_utf16(id),
        .url = platform::win32::to_utf16(std::format("https://music.youtube.com/watch?v={}", id)),
        .chosen_by = chosen_by,
    };
}

void show_entry_audio(
        const core::State& state, const platform::TextNormalizer& normalizer, const core::Track& track, const std::string& entry, TrackDetails& details
) {
    // How the entry itself got its YouTube Music id, so a duplicate shows what the Matcher did with it
    const std::string* first_spotify = core::first_source_of(track, core::SourceType::Spotify);
    // The user's choice is every entry's
    if (!track.ytm_override.empty()) {
        details.audio = audio(track.ytm_override, AudioChooser::Override);
        details.match = match_of(state, normalizer, core::source_type_of_key(entry) == core::SourceType::Spotify ? &entry : first_spotify);
        return;
    }
    if (std::optional<std::string> own = core::own_ytm_id(entry)) {
        details.audio = audio(*own, AudioChooser::Playlist);
        details.match = match_of(state, normalizer, first_spotify);
        return;
    }
    if (auto source = state.sources.find(entry); source != state.sources.end() && source->second.match) {
        details.audio = audio(source->second.match->ytm_id, AudioChooser::Matcher);
        details.match = match_of(state, normalizer, &entry);
        return;
    }
    if (std::optional<core::ResolvedYtmId> resolved = core::resolved_ytm_id(state, track)) {
        bool chosen_by_playlist = core::source_type_of_key(resolved->source) == core::SourceType::YouTubeMusic;
        details.audio = audio(resolved->ytm_id, chosen_by_playlist ? AudioChooser::Playlist : AudioChooser::Matcher);
    }
    details.match = match_of(state, normalizer, first_spotify);
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

std::optional<TrackDetails> track_details(
        const core::State& state, const platform::TextNormalizer& normalizer, const TrackRow& row, const std::filesystem::path& library_folder
) {
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
    show_entry_audio(state, normalizer, track->second, row.key, details);
    if (track->second.status == core::TrackStatus::Failed) {
        details.failure = track->second.failure;
        details.error = last_lines(track->second.error);
    }
    details.year = core::effective_metadata(state, track->second).year;
    if (!track->second.file.empty()) {
        details.file = library_folder / track->second.file;
    }
    return details;
}

}
