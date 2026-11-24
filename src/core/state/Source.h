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
    // A YouTube Music source gets its album and year when it is downloaded
    std::optional<int> year{};
    std::chrono::milliseconds duration{};
    // Known for Spotify sources only
    std::optional<bool> explicit_content{};
    // Spotify sources only
    std::string cover_url{};
    std::optional<Match> match{};
    std::string track{};
};

}
