#pragma once

#include <chrono>
#include <optional>
#include <string>
#include <vector>

namespace westie::core {

struct SourceTrack {
    std::string key{};
    std::string title{};
    std::vector<std::string> artists{};
    // Only Spotify's playlist has it; a YouTube Music source gets its album when its video is downloaded
    std::optional<std::string> album{};
    std::chrono::milliseconds duration{};
};

struct PlaylistContents {
    std::string name{};
    std::vector<SourceTrack> tracks{};
};

}
