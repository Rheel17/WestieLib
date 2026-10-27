#include "ui/tracks/TrackRows.h"

#include <algorithm>
#include <map>
#include <ranges>
#include <set>
#include <string_view>
#include <utility>

#include "core/state/EffectiveMetadata.h"
#include "platform/win32/Utf16.h"
#include "ui/playlists/PlaylistRow.h"
#include "ui/playlists/PlaylistRows.h"

namespace westie::ui {

namespace {

constexpr std::wstring_view Separator = L", ";

struct Listing {
    std::string track{};
    int position = 0;
};

StatusTab status_tab(core::TrackStatus status) {
    switch (status) {
    case core::TrackStatus::Downloaded: return StatusTab::Synced;
    case core::TrackStatus::Failed:     return StatusTab::Failed;
    case core::TrackStatus::Orphaned:   return StatusTab::Removed;
    case core::TrackStatus::Kept:       return StatusTab::Kept;
    default:                            return StatusTab::Waiting;
    }
}

std::wstring join(const std::vector<std::wstring>& parts) {
    std::wstring text;
    for (const std::wstring& part : parts) {
        if (!text.empty()) {
            text += Separator;
        }
        text += part;
    }
    return text;
}

std::wstring join_utf8(const std::vector<std::string>& parts) {
    std::vector<std::wstring> wide;
    for (const std::string& part : parts) {
        wide.push_back(platform::win32::to_utf16(part));
    }
    return join(wide);
}

// The track each source belongs to, in the order the playlist lists them, each track once
std::vector<Listing> listings(const core::State& state, const core::Playlist& playlist) {
    std::vector<Listing> found;
    std::set<std::string> seen;
    int position = 0;
    for (const std::string& key : playlist.sources) {
        ++position;
        auto source = state.sources.find(key);
        if (source != state.sources.end() && seen.insert(source->second.track).second) {
            found.push_back({.track = source->second.track, .position = position});
        }
    }
    return found;
}

std::map<std::string, std::vector<std::wstring>> playlist_names(const core::State& state) {
    std::map<std::string, std::vector<std::wstring>> names;
    for (const PlaylistRow& row : playlist_rows(state)) {
        for (const Listing& listing : listings(state, state.playlists.at(row.key))) {
            names[listing.track].push_back(row.name);
        }
    }
    return names;
}

}

std::vector<TrackRow> track_rows(const core::State& state, const std::optional<std::string>& playlist) {
    std::vector<Listing> shown;
    if (playlist) {
        if (auto listed = state.playlists.find(*playlist); listed != state.playlists.end()) {
            shown = listings(state, listed->second);
        }
    } else {
        for (const std::string& key : state.tracks | std::views::keys) {
            shown.push_back({.track = key});
        }
    }

    std::map<std::string, std::vector<std::wstring>> names = playlist_names(state);
    std::vector<TrackRow> rows;
    for (Listing& listing : shown) {
        auto track = state.tracks.find(listing.track);
        if (track == state.tracks.end()) {
            continue;
        }
        TrackRow row{.status = status_tab(track->second.status), .position = listing.position};
        core::TrackMetadata metadata = core::effective_metadata(state, track->second);
        row.title = platform::win32::to_utf16(metadata.title);
        row.artists = join_utf8(metadata.artists);
        row.album = platform::win32::to_utf16(metadata.album);
        row.duration = metadata.duration;
        for (const std::string& source : track->second.sources) {
            std::optional<core::SourceType> type = core::source_type_of_key(source);
            if (type && !std::ranges::contains(row.sources, *type)) {
                row.sources.push_back(*type);
            }
        }
        row.playlists = join(names[listing.track]);
        row.key = std::move(listing.track);
        rows.push_back(std::move(row));
    }
    return rows;
}

std::vector<TrackRow> rows_from_source(const std::vector<TrackRow>& rows, std::optional<core::SourceType> source) {
    if (!source) {
        return rows;
    }
    return rows | std::views::filter([&](const TrackRow& row) { return std::ranges::contains(row.sources, *source); }) | std::ranges::to<std::vector>();
}

std::vector<TrackRow> rows_in_tab(const std::vector<TrackRow>& rows, StatusTab tab) {
    auto in_tab = [tab](const TrackRow& row) {
        switch (tab) {
        case StatusTab::All:    return true;
        // Needs review is part of Synced
        case StatusTab::Synced: return row.status == StatusTab::Synced || row.status == StatusTab::NeedsReview;
        default:                return row.status == tab;
        }
    };
    return rows | std::views::filter(in_tab) | std::ranges::to<std::vector>();
}

StatusCounts status_counts(const std::vector<TrackRow>& rows) {
    StatusCounts counts;
    for (const TrackRow& row : rows) {
        ++counts.all;
        switch (row.status) {
        case StatusTab::NeedsReview: ++counts.needs_review; ++counts.synced; break;
        case StatusTab::Synced:      ++counts.synced; break;
        case StatusTab::Waiting:     ++counts.waiting; break;
        case StatusTab::Failed:      ++counts.failed; break;
        case StatusTab::Removed:     ++counts.removed; break;
        case StatusTab::Kept:        ++counts.kept; break;
        default:                     break;
        }
    }
    return counts;
}

}
