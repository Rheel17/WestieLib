#pragma once

#include <string>
#include <vector>

#include "core/state/State.h"

namespace westie::core {

bool needs_match(const State& state, const Track& track);
std::vector<std::string> tracks_to_match(const State& state);

}
