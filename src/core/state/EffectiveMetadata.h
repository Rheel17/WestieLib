#pragma once

#include <chrono>
#include <optional>
#include <string>
#include <vector>

#include "core/state/State.h"

namespace westie::core {

struct TrackMetadata {
    std::string title{};
    std::vector<std::string> artists{};
    std::string album{};
    std::optional<int> year{};
    std::chrono::milliseconds duration{};
    std::string cover_url{};
};

TrackMetadata effective_metadata(const State& state, const Track& track);

}
