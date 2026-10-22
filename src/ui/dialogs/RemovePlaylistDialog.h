#pragma once

#include <string_view>

#include <windows.h>

namespace westie::ui {

bool confirm_remove_playlist(HWND owner, std::wstring_view name);

}
