#pragma once

#include <chrono>
#include <optional>
#include <string>
#include <vector>

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
};

std::vector<TrackRow> track_rows(const core::State& state, const std::optional<std::string>& playlist);
StatusCounts status_counts(const std::vector<TrackRow>& rows);

}
