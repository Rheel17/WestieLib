#pragma once

#include <chrono>
#include <string>
#include <vector>

namespace westie::core {

struct Match {
    std::string video_id{};
    // From 0 to 1
    double confidence = 0.0;
    std::string title{};
    std::vector<std::string> artists{};
    std::chrono::milliseconds duration{};
};

}
