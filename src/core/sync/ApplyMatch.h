#pragma once

#include <optional>
#include <string>

#include "core/state/Match.h"
#include "core/state/State.h"

namespace westie::core {

std::optional<std::string> apply_match(State& state, const std::string& track_key, const Match& match);
void apply_match_failure(State& state, const std::string& track_key, TrackFailure failure, std::string error);

}
