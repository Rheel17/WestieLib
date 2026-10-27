#pragma once

#include <string>
#include <vector>

#include "core/state/Match.h"
#include "core/state/State.h"
#include "platform/TextNormalizer.h"

namespace westie::core {

struct LibraryMatch {
    std::string track{};
    Match match{};
};

std::vector<LibraryMatch> library_matches(const State& state, const platform::TextNormalizer& normalizer);

}
