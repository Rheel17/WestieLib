#pragma once

#include <chrono>

namespace westie::core {

struct ToolsChecked {
    std::chrono::system_clock::time_point checked_at{};
};

}
