#pragma once

#include <optional>
#include <string>

#include "core/state/State.h"

namespace westie::ui {

struct TrackCounts {
    int total = 0;
    int synced = 0;
    int waiting = 0;
    int failed = 0;
    int to_review = 0;
};

TrackCounts count_tracks(const core::State& state, const std::optional<std::string>& playlist);

}
