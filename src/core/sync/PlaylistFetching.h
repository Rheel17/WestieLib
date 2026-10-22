#pragma once

#include <string>
#include <vector>

#include "core/tools/ToolPackage.h"

namespace westie::core {

struct PlaylistFetching {
    std::string playlist{};
    // Empty while it fetches
    std::vector<ToolPackage> waiting_for{};
};

}
