#pragma once

#include <functional>
#include <vector>

#include "core/state/State.h"
#include "core/sync/TracksQueued.h"

namespace westie::core {

std::vector<QueuedTrack> tracks_in_playlist_order(const State& state, const std::function<bool(const Track& track)>& wanted);

}
