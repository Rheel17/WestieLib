#pragma once

#include <string>
#include <vector>

#include "core/state/Track.h"
#include "core/tools/ToolPackage.h"

namespace westie::core {

struct TrackStatusChanged {
    std::string track{};
    TrackStatus status = TrackStatus::Pending;
    TrackFailure failure = TrackFailure::None;
    std::string error{};
    // A track that waits to download until these are installed
    std::vector<ToolPackage> waiting_for{};
};

}
