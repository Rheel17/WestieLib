#pragma once

#include <string>
#include <vector>

#include "core/state/State.h"

namespace westie::ui {

struct TrackEntry {
    std::string source{};
    std::string track{};
};

std::vector<TrackEntry> track_entries(const core::State& state);

}
