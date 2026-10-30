#pragma once

#include <chrono>
#include <optional>
#include <string>
#include <vector>

#include "core/state/Match.h"

namespace westie::core {

struct Source {
    std::string title{};
    std::vector<std::string> artists{};
    std::string album{};
    std::chrono::milliseconds duration{};
    // Known for Spotify sources only
    std::optional<bool> explicit_content{};
    // Spotify sources only
    std::optional<Match> match{};
    std::string track{};
};

}
