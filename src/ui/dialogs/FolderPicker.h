#pragma once

#include <filesystem>
#include <optional>

#include <windows.h>

namespace westie::ui {

std::optional<std::filesystem::path> pick_folder(HWND owner, const wchar_t* title, const std::filesystem::path& start);

}
