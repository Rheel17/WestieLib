#pragma once

#include <chrono>
#include <string>
#include <vector>

namespace westie::core {

enum class TrackStatus { Pending, Downloaded, Failed, Orphaned, Kept };

enum class TrackFailure { None, NoMatch, Search };

struct Track {
    // The first one created the track, and its key is the track's
    std::vector<std::string> sources{};
    // A video the user chose, which wins over the sources'
    std::string video_override{};
    TrackStatus status = TrackStatus::Pending;
    // Set with Failed
    TrackFailure failure = TrackFailure::None;
    std::string error{};
    std::chrono::system_clock::time_point added_at{};
};

}
