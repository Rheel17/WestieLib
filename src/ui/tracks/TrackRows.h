#pragma once

#include <chrono>
#include <optional>
#include <string>
#include <vector>

#include "core/sources/SourceType.h"
#include "core/state/State.h"
#include "ui/tracks/StatusCounts.h"

namespace westie::ui {

struct TrackRow {
    std::string key{};
    StatusTab status = StatusTab::Waiting;
    std::wstring title{};
    std::wstring artists{};
    std::wstring album{};
    std::wstring playlists{};
    std::chrono::milliseconds duration{};
    // Where the scope's playlist first lists the track, from 1; 0 in All tracks
    int position = 0;
    // Each kind of source the track has, once
    std::vector<core::SourceType> sources{};
};

std::vector<TrackRow> track_rows(const core::State& state, const std::optional<std::string>& playlist);
std::vector<TrackRow> rows_from_source(const std::vector<TrackRow>& rows, std::optional<core::SourceType> source);
std::vector<TrackRow> rows_in_tab(const std::vector<TrackRow>& rows, StatusTab tab);
StatusCounts status_counts(const std::vector<TrackRow>& rows);

}
