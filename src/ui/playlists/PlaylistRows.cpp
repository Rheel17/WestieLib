#include "ui/playlists/PlaylistRows.h"

#include <algorithm>
#include <ranges>
#include <string>
#include <string_view>
#include <tuple>
#include <utility>

#include "platform/win32/Utf16.h"
#include "ui/PlaylistTypeText.h"

namespace westie::ui {

namespace {

std::optional<PlaylistRow> row_of(const std::string& key, const core::Playlist& playlist) {
    std::optional<core::SourceType> type = core::source_type_of_key(key);
    // A playlist from a later version's source can't be shown
    if (!type) {
        return std::nullopt;
    }
    // Until its first fetch a playlist has no name
    std::wstring name = playlist.name.empty() ? playlist_type_text(*type) : platform::win32::to_utf16(playlist.name);
    // The key is the source's prefix, a colon and the playlist's ID
    std::wstring id = platform::win32::to_utf16(std::string_view(key).substr(core::source_prefix(*type).size() + 1));
    return PlaylistRow{.key = key, .type = *type, .name = std::move(name), .id = std::move(id), .url = platform::win32::to_utf16(playlist.url)};
}

}

std::vector<PlaylistRow> playlist_rows(const core::State& state) {
    auto entries = state.playlists | std::views::transform([](const auto& entry) { return &entry; }) | std::ranges::to<std::vector>();
    // In the order they were added; the key only breaks ties
    std::ranges::sort(entries, {}, [](const auto* entry) { return std::tie(entry->second.added_at, entry->first); });

    std::vector<PlaylistRow> rows;
    for (const auto* entry : entries) {
        if (std::optional<PlaylistRow> row = row_of(entry->first, entry->second)) {
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
    return row_of(playlist->first, playlist->second);
}

}
