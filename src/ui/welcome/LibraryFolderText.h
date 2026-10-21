#pragma once

#include <filesystem>
#include <optional>
#include <string_view>

namespace westie::ui {

std::optional<std::filesystem::path> parse_library_folder(std::wstring_view text);

}
