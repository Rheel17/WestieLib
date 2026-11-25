#pragma once

#include <optional>
#include <string>

#include "core/library/Library.h"
#include "core/log/Log.h"

namespace westie::core {

// Gives the source's track, or a track of the source's own, the YouTube Music id. Returns the track it merged into, if any.
std::optional<std::string> set_ytm_override(Library& library, Log& log, const std::string& source_key, const std::string& ytm_id);

}
