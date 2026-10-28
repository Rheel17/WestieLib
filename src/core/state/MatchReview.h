#pragma once

#include "core/state/Match.h"
#include "core/state/State.h"

namespace westie::core {

// Still being tuned: a lower one lets more wrong matches through, a higher one asks about more right ones
constexpr double ReviewThreshold = 0.85;

bool needs_review(const Match& match);
bool needs_review(const State& state, const Track& track);

}
