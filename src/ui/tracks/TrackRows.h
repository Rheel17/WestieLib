#pragma once

#include <chrono>
#include <optional>
#include <string>
#include <vector>

#include "core/sources/SourceType.h"
#include "core/state/State.h"
#include "platform/TextNormalizer.h"
#include "ui/tracks/StatusCounts.h"

namespace westie::ui {

struct RowOriginal {
    // In the same playlist
    int position = 0;
};

struct TrackRow {
    // The source key of the entry, or in All tracks of the track's first entry
    std::string key{};
    std::string track{};
    // Its track's status, a duplicate's too
    StatusTab status = StatusTab::Waiting;
    std::wstring title{};
    std::wstring artists{};
    std::wstring album{};
    std::wstring playlists{};
    std::chrono::milliseconds duration{};
    // The title, artists and album, normalized for the search
    std::string search_text{};
    // Where the scope's playlist first lists the entry, from 1; 0 in All tracks
    int position = 0;
    // The kind of source the row is for: the entry's, or in All tracks the track's first entry's
    std::optional<core::SourceType> source{};
    // Set when the playlist lists the same track earlier, through another source
    std::optional<RowOriginal> duplicate_of{};
};

std::vector<TrackRow> track_rows(const core::State& state, const std::optional<std::string>& playlist, const platform::TextNormalizer& normalizer);
std::vector<TrackRow> rows_from_source(const std::vector<TrackRow>& rows, std::optional<core::SourceType> source);
std::vector<TrackRow> rows_in_tab(const std::vector<TrackRow>& rows, StatusTab tab);
StatusCounts status_counts(const std::vector<TrackRow>& rows);

}
