#pragma once

#include <vector>

#include "core/state/State.h"
#include "core/sync/TracksQueued.h"

namespace westie::core {

bool needs_download(const State& state, const Track& track);
std::vector<QueuedTrack> tracks_to_download(const State& state);

}
