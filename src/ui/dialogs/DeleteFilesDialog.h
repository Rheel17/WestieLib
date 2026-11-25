#pragma once

#include <string>
#include <vector>

#include <windows.h>

namespace westie::ui {

bool confirm_delete_files(HWND owner, const std::vector<std::wstring>& files);

}
