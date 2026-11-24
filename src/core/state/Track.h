#pragma once

#include <chrono>
#include <filesystem>
#include <string>
#include <vector>

namespace westie::core {

enum class TrackStatus { Pending, Downloaded, Failed, Orphaned, Kept };

enum class TrackFailure { None, NoMatch, Search, Download };

struct Track {
    // The first one created the track, and its key is the track's
    std::vector<std::string> sources{};
    // The YouTube Music id the file was made from
    std::string ytm_id{};
    // A YouTube Music id the user chose, which wins over the sources'
    std::string ytm_override{};
    TrackStatus status = TrackStatus::Pending;
    // Set with Failed
    TrackFailure failure = TrackFailure::None;
    std::string error{};
    // Relative to the library folder
    std::filesystem::path file{};
    std::chrono::system_clock::time_point added_at{};
};

}
