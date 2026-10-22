#pragma once

#include <string_view>

#include <windows.h>

namespace westie::platform::win32 {

bool set_clipboard_text(HWND owner, std::wstring_view text);

}
