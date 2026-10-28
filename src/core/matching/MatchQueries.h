#pragma once

#include <string>
#include <vector>

#include "core/state/Source.h"

namespace westie::core {

// What the Matcher searches for a source, in order
std::vector<std::string> match_queries(const Source& source);

}
