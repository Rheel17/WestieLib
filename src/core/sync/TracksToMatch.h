#pragma once

#include <string>
#include <vector>

#include "core/state/State.h"
#include "core/sync/TracksQueued.h"

namespace westie::core {

bool needs_match(const State& state, const Track& track);
std::vector<QueuedTrack> tracks_to_match(const State& state);

}
