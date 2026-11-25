#pragma once

#include <filesystem>
#include <string>

#include <windows.h>

namespace westie::platform::win32 {

bool open_with_shell(HWND owner, const std::wstring& target) noexcept;
// Opens Explorer on the file's folder with the file selected
bool show_in_folder(const std::filesystem::path& file) noexcept;

}
