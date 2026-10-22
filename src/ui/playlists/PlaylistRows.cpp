#include "ui/playlists/PlaylistRows.h"

#include <algorithm>
#include <optional>
#include <ranges>
#include <string>
#include <tuple>
#include <utility>

#include "platform/win32/Utf16.h"
#include "ui/PlaylistTypeText.h"

namespace westie::ui {

std::vector<PlaylistRow> playlist_rows(const core::State& state) {
    auto entries = state.playlists | std::views::transform([](const auto& entry) { return &entry; }) | std::ranges::to<std::vector>();
    // In the order they were added; the key only breaks ties
    std::ranges::sort(entries, {}, [](const auto* entry) { return std::tie(entry->second.added_at, entry->first); });

    std::vector<PlaylistRow> rows;
    for (const auto* entry : entries) {
        // A playlist from a later version's source can't be shown
        std::optional<core::SourceType> type = core::source_type_of_key(entry->first);
        if (!type) {
            continue;
        }
        const core::Playlist& playlist = entry->second;
        // Until its first fetch a playlist has no name
        std::wstring name = playlist.name.empty() ? playlist_type_text(*type) : platform::win32::to_utf16(playlist.name);
        rows.push_back({.key = entry->first, .type = *type, .name = std::move(name)});
    }
    return rows;
}

}
