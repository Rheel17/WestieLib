#pragma once

#include <map>
#include <string>

#include "core/state/State.h"

namespace westie::core {

enum class RemovalDecision { Keep, Remove };

std::map<std::string, RemovalDecision> apply_removals(State& state, const std::map<std::string, RemovalDecision>& decisions);

}