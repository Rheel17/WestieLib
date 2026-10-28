#pragma once

#include <optional>
#include <string>

#include "core/state/State.h"
#include "core/state/StateStore.h"

namespace westie::core {

// The track the confirmed one merged into, if any
std::optional<std::string> confirm_match(State& state, const std::string& track_key);
std::optional<std::string> confirm_match(StateStore& store, const std::string& track_key);

}
