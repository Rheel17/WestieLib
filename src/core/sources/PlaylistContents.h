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
    // Only Spotify's playlist has it; a YouTube Music source gets its album when it is downloaded
    std::optional<std::string> album{};
    std::chrono::milliseconds duration{};
    // Only Spotify's playlist has these
    std::optional<bool> explicit_content{};
    std::optional<std::string> cover_url{};
};

struct PlaylistContents {
    std::string name{};
    std::vector<SourceTrack> tracks{};
};

}
