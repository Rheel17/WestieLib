#pragma once

#include <set>
#include <string>

#include "core/state/State.h"

namespace westie::core {

// The sources some playlist lists
std::set<std::string> listed_sources(const State& state);

}
