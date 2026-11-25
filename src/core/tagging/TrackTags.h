#pragma once

#include <optional>
#include <string>
#include <vector>

namespace westie::core {

struct TrackTags {
    std::string title{};
    std::vector<std::string> artists{};
    std::string album{};
    std::optional<int> year{};
    // Every source key of the track, the track key first
    std::vector<std::string> sources{};
    std::string ytm_id{};
    // A JPEG, or empty for no cover
    std::string cover{};

    bool operator==(const TrackTags&) const = default;
};

}
