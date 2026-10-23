#include "ui/playlists/PlaylistRows.h"

#include <algorithm>
#include <ranges>
#include <set>
#include <string>
#include <string_view>
#include <tuple>
#include <utility>

#include "platform/win32/Utf16.h"
#include "ui/PlaylistTypeText.h"

namespace westie::ui {

namespace {

std::set<std::string> listed_tracks(const core::State& state, const core::Playlist& playlist) {
    std::set<std::string> tracks;
    for (const std::string& source : playlist.sources) {
        if (auto entry = state.sources.find(source); entry != state.sources.end()) {
            tracks.insert(entry->second.track);
        }
    }
    return tracks;
}

int shared_tracks(const core::State& state, const std::string& key, const core::Playlist& playlist, core::SourceType type) {
    std::set<std::string> elsewhere;
    for (const auto& [other_key, other] : state.playlists) {
        if (other_key != key && core::source_type_of_key(other_key) == type) {
            elsewhere.merge(listed_tracks(state, other));
        }
    }
    return static_cast<int>(std::ranges::count_if(listed_tracks(state, playlist), [&](const std::string& track) { return elsewhere.contains(track); }));
}

std::optional<PlaylistRow> row_of(const core::State& state, const std::string& key, const core::Playlist& playlist) {
    std::optional<core::SourceType> type = core::source_type_of_key(key);
    // A playlist from a later version's source can't be shown
    if (!type) {
        return std::nullopt;
    }
    // Until its first fetch a playlist has no name
    std::wstring name = playlist.name.empty() ? playlist_type_text(*type) : platform::win32::to_utf16(playlist.name);
    // The key is the source's prefix, a colon and the playlist's ID
    std::wstring id = platform::win32::to_utf16(std::string_view(key).substr(core::source_prefix(*type).size() + 1));
    std::optional<std::chrono::system_clock::time_point> failed_at;
    if (!playlist.fetch_error.empty()) {
        failed_at = playlist.fetch_error_at.value_or(std::chrono::system_clock::time_point{});
    }
    return PlaylistRow{
        .key = key,
        .type = *type,
        .name = std::move(name),
        .id = std::move(id),
        .url = platform::win32::to_utf16(playlist.url),
        .counts = count_tracks(state, key),
        .shared = shared_tracks(state, key, playlist, *type),
        .last_sync = playlist.last_sync,
        .fetch_failed_at = failed_at,
    };
}

}

std::vector<PlaylistRow> playlist_rows(const core::State& state) {
    auto entries = state.playlists | std::views::transform([](const auto& entry) { return &entry; }) | std::ranges::to<std::vector>();
    // In the order they were added; the key only breaks ties
    std::ranges::sort(entries, {}, [](const auto* entry) { return std::tie(entry->second.added_at, entry->first); });

    std::vector<PlaylistRow> rows;
    for (const auto* entry : entries) {
        if (std::optional<PlaylistRow> row = row_of(state, entry->first, entry->second)) {
            rows.push_back(std::move(*row));
        }
    }
    return rows;
}

std::optional<PlaylistRow> playlist_row(const core::State& state, const std::string& key) {
    auto playlist = state.playlists.find(key);
    if (playlist == state.playlists.end()) {
        return std::nullopt;
    }
    return row_of(state, playlist->first, playlist->second);
}

void mark_runs(std::vector<PlaylistRow>& rows, const SyncBoard& board) {
    for (PlaylistRow& row : rows) {
        row.run = PlaylistRun::None;
        row.waiting_for.clear();
        if (board.current() == row.key) {
            row.run = PlaylistRun::Fetching;
            row.waiting_for.assign(board.waiting_for().begin(), board.waiting_for().end());
        } else if (board.queued(row.key)) {
            row.run = PlaylistRun::Queued;
        }
    }
}

}
