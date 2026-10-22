#pragma once

#include <chrono>
#include <string>

namespace westie::core {

struct Playlist {
    std::string url{};
    std::string name{};
    std::chrono::system_clock::time_point added_at{};
};

}
