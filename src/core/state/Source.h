#pragma once

#include <chrono>
#include <string>
#include <vector>

namespace westie::core {

struct Source {
    std::string title{};
    std::vector<std::string> artists{};
    std::chrono::milliseconds duration{};
    std::string track{};
};

}
