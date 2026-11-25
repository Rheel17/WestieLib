#pragma once

#include <map>
#include <optional>
#include <string>
#include <vector>

#include "core/tools/ToolPackage.h"

namespace westie::ui {

// What an update does with a track right now, which the state doesn't know
struct TrackActivity {
    // While it downloads
    std::optional<int> percent{};
    // While its download waits for tools
    std::vector<core::ToolPackage> waiting_for{};

    bool operator==(const TrackActivity&) const = default;
};

// By track key
using TrackActivities = std::map<std::string, TrackActivity>;

}
