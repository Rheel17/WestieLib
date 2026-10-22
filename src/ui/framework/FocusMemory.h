#pragma once

#include <windows.h>

namespace westie::ui::framework {

class FocusMemory {
public:
    void save(HWND window) noexcept;
    void restore(HWND window) const noexcept;

private:
    HWND focus_ = nullptr;
};

}
