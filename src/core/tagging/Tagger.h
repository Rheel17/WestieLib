#pragma once

#include <filesystem>

#include "core/tagging/TrackTags.h"

namespace westie::core {

void write_tags(const std::filesystem::path& file, const TrackTags& tags);

}
