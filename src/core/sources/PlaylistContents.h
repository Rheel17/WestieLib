#pragma once

#include <chrono>
#include <string>
#include <vector>

namespace westie::core {

struct SourceTrack {
    std::string key{};
    std::string title{};
    std::vector<std::string> artists{};
    std::chrono::milliseconds duration{};
};

struct PlaylistContents {
    std::string name{};
    std::vector<SourceTrack> tracks{};
};

}
