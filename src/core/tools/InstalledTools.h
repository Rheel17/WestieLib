#pragma once

#include <chrono>
#include <map>
#include <optional>
#include <string>

#include "core/tools/ToolPackage.h"

namespace westie::core {

struct InstalledTool {
    std::string release{};
    std::string digest{};
};

struct InstalledTools {
    std::map<ToolPackage, InstalledTool> tools{};
    std::optional<std::chrono::system_clock::time_point> checked_at{};
};

}
