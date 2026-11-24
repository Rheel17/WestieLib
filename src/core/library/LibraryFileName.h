#pragma once

#include <set>
#include <string>
#include <string_view>

#include "core/state/EffectiveMetadata.h"

namespace westie::core {

std::string library_file_stem(const TrackMetadata& metadata, std::string_view ytm_id);
std::string numbered_file_name(std::string_view stem, std::string_view extension, int number);
std::string file_name_key(std::string_view name);
std::string free_file_name(std::string_view stem, std::string_view extension, const std::set<std::string>& taken_keys, int first_number = 1);

}
