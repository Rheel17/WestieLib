#pragma once

#include <filesystem>
#include <string>

#include "core/tagging/TrackTags.h"

namespace westie::core {

void write_tags(const std::filesystem::path& file, const TrackTags& tags);
std::string read_cover(const std::filesystem::path& file);

}
