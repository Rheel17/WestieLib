#pragma once

#include <span>

#include <windows.h>

#include "platform/win32/UniqueHandle.h"

namespace westie::ui::framework {

struct MenuItem {
    WORD command = 0;
    const wchar_t* text = nullptr;
};

constexpr MenuItem MenuSeparator{};

platform::win32::UniqueMenu build_popup_menu(std::span<const MenuItem> items);

}
