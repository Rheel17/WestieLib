#pragma once

#include <filesystem>
#include <string>
#include <string_view>

namespace westie::core {

std::string to_utf8(const std::filesystem::path& path);
std::filesystem::path path_from_utf8(std::string_view text);

}
