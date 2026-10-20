#pragma once

#include <windows.h>

namespace westie::ui::framework {

inline HINSTANCE module_instance() noexcept {
    return GetModuleHandleW(nullptr);
}

}
