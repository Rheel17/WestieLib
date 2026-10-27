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
#include "ui/tracks/TrackEntries.h"

namespace westie::ui {

namespace {

constexpr std::wstring_view Separator = L", ";

struct Listing {
    std::string source{};
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

// The sources the playlist lists, in its order, each once
std::vector<Listing> listings(const core::Playlist& playlist) {
    std::vector<Listing> found;
    std::set<std::string> seen;
    int position = 0;
    for (const std::string& key : playlist.sources) {
        ++position;
        if (seen.insert(key).second) {
            found.push_back({.source = key, .position = position});
        }
    }
    return found;
}

// By track, the playlists that list one of its sources, in the left panel's order
std::map<std::string, std::vector<std::wstring>> playlist_names(const core::State& state) {
    std::map<std::string, std::vector<std::wstring>> names;
    for (const PlaylistRow& row : playlist_rows(state)) {
        std::set<std::string> tracks;
        for (const std::string& key : state.playlists.at(row.key).sources) {
            if (auto source = state.sources.find(key); source != state.sources.end() && tracks.insert(source->second.track).second) {
                names[source->second.track].push_back(row.name);
            }
        }
    }
    return names;
}

// Each track once, by its first entry
std::vector<Listing> first_entries(const core::State& state) {
    std::vector<Listing> found;
    std::set<std::string> tracks;
    for (TrackEntry& entry : track_entries(state)) {
        if (tracks.insert(entry.track).second) {
            found.push_back({.source = std::move(entry.source)});
        }
    }
    return found;
}

}

std::vector<TrackRow> track_rows(const core::State& state, const std::optional<std::string>& playlist) {
    std::vector<Listing> shown;
    if (playlist) {
        if (auto listed = state.playlists.find(*playlist); listed != state.playlists.end()) {
            shown = listings(listed->second);
        }
    } else {
        shown = first_entries(state);
    }

    std::map<std::string, std::vector<std::wstring>> names = playlist_names(state);
    // By track, where the playlist first lists it: a later entry of the same track is a duplicate of that one
    std::map<std::string, int> originals;
    std::vector<TrackRow> rows;
    for (Listing& listing : shown) {
        auto source = state.sources.find(listing.source);
        if (source == state.sources.end()) {
            continue;
        }
        auto track = state.tracks.find(source->second.track);
        if (track == state.tracks.end()) {
            continue;
        }
        TrackRow row{
            .track = track->first,
            .status = status_tab(track->second.status),
            .position = listing.position,
            .source = core::source_type_of_key(listing.source),
        };
        core::TrackMetadata metadata = core::effective_metadata(state, track->second);
        row.title = platform::win32::to_utf16(metadata.title);
        row.artists = join_utf8(metadata.artists);
        row.album = platform::win32::to_utf16(metadata.album);
        row.duration = metadata.duration;
        row.playlists = join(names[track->first]);
        if (auto [original, first] = originals.try_emplace(track->first, listing.position); playlist && !first) {
            row.duplicate_of = RowOriginal{.position = original->second};
        }
        row.key = std::move(listing.source);
        rows.push_back(std::move(row));
    }
    return rows;
}

std::vector<TrackRow> rows_from_source(const std::vector<TrackRow>& rows, std::optional<core::SourceType> source) {
    if (!source) {
        return rows;
    }
    return rows | std::views::filter([&](const TrackRow& row) { return row.source == source; }) | std::ranges::to<std::vector>();
}

std::vector<TrackRow> rows_in_tab(const std::vector<TrackRow>& rows, StatusTab tab) {
    auto in_tab = [tab](const TrackRow& row) {
        switch (tab) {
        case StatusTab::All:    return true;
        // Needs review is part of Synced
        case StatusTab::Synced:     return row.status == StatusTab::Synced || row.status == StatusTab::NeedsReview;
        case StatusTab::Duplicates: return row.duplicate_of.has_value();
        default:                    return row.status == tab;
        }
    };
    return rows | std::views::filter(in_tab) | std::ranges::to<std::vector>();
}

StatusCounts status_counts(const std::vector<TrackRow>& rows) {
    StatusCounts counts;
    for (const TrackRow& row : rows) {
        ++counts.all;
        if (row.duplicate_of) {
            ++counts.duplicates;
        }
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
