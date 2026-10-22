#pragma once

#include <chrono>
#include <string>
#include <vector>

namespace westie::core {

enum class TrackStatus { Pending, Downloaded, Failed, Orphaned, Kept };

struct Track {
    // The first one created the track, and its key is the track's
    std::vector<std::string> sources{};
    TrackStatus status = TrackStatus::Pending;
    std::chrono::system_clock::time_point added_at{};
};

}
