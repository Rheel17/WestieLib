#pragma once

#include <optional>
#include <string>

#include "core/state/StateStore.h"

namespace westie::core {

// Gives the source's track, or a track of the source's own, the video. Returns the track it merged into, if any.
std::optional<std::string> set_video_override(StateStore& store, const std::string& source_key, const std::string& video_id);

}
