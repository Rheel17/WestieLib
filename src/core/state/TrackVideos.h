#pragma once

#include <optional>
#include <set>
#include <string>
#include <string_view>

#include "core/sources/SourceType.h"
#include "core/state/State.h"

namespace westie::core {

struct ResolvedVideo {
    std::string video_id{};
    // A YouTube Music source, whose own video it is, or a Spotify source, whose match it is
    std::string source{};
};

const std::string* first_source_of(const Track& track, SourceType type);
std::optional<std::string> own_video(std::string_view source_key);
std::optional<ResolvedVideo> resolved_video(const State& state, const Track& track);
std::optional<std::string> track_with_video(const State& state, std::string_view video_id, std::string_view except = {});
std::set<std::string> track_videos(const State& state);

}
