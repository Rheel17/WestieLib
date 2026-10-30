#pragma once

#include <chrono>
#include <optional>
#include <string>
#include <vector>

#include "core/state/MatchDoubt.h"

namespace westie::core {

struct Match {
    std::string video_id{};
    // From 0 to 1
    double confidence = 0.0;
    bool confirmed = false;
    // Why the Matcher wasn't sure, when it wasn't
    std::vector<MatchDoubt> doubts{};
    std::string title{};
    std::vector<std::string> artists{};
    std::string album{};
    std::chrono::milliseconds duration{};
    std::optional<bool> explicit_content{};
};

}
