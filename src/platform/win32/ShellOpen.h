#pragma once

#include <string>

#include <windows.h>

namespace westie::platform::win32 {

bool open_with_shell(HWND owner, const std::wstring& target) noexcept;

}
