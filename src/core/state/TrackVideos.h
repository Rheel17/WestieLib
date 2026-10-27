#pragma once

#include <optional>
#include <string>

#include "core/state/State.h"

namespace westie::core {

struct ResolvedVideo {
    std::string video_id{};
    // A YouTube Music source, whose own video it is, or a Spotify source, whose match it is
    std::string source{};
};

std::optional<ResolvedVideo> resolved_video(const State& state, const Track& track);

}
