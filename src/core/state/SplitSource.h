#pragma once

#include <string>

#include "core/state/State.h"

namespace westie::core {

// Leaves the source alone in a track keyed by it, the others in a track of their own
void split_source(State& state, const std::string& source_key);

}
