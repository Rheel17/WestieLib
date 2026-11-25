#pragma once

#include "core/state/State.h"

namespace westie::core {

bool is_listed(const State& state, const Track& track);
TrackStatus returning_status(const State& state, const Track& track);
void prune_tracks(State& state);

}
