#pragma once

#include <chrono>
#include <string>
#include <vector>

#include "core/state/State.h"

namespace westie::core {

struct TrackMetadata {
    std::string title{};
    std::vector<std::string> artists{};
    std::string album{};
    std::chrono::milliseconds duration{};
};

TrackMetadata effective_metadata(const State& state, const Track& track);

}
