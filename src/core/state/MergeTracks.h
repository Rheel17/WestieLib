#pragma once

#include <string>

#include "core/state/State.h"

namespace westie::core {

void merge_tracks(State& state, const std::string& survivor, const std::string& merged);

}
