#pragma once

#include <chrono>
#include <optional>
#include <string>
#include <vector>

namespace westie::core {

struct Playlist {
    std::string url{};
    std::string name{};
    std::chrono::system_clock::time_point added_at{};
    // Source keys in the playlist's order; a playlist can list the same one twice
    std::vector<std::string> sources{};
    std::optional<std::chrono::system_clock::time_point> last_sync{};
    std::string fetch_error{};
    std::optional<std::chrono::system_clock::time_point> fetch_error_at{};
};

}
