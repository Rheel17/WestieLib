#pragma once

#include <windows.h>

namespace westie::ui::framework {

bool handle_edit_key(HWND edit, UINT message, WPARAM wparam);
void add_edit_keys(HWND edit);

}
