#pragma once

#include <span>

#include <windows.h>

#include "platform/win32/UniqueHandle.h"

namespace westie::ui::framework {

class AcceleratorTable {
public:
    explicit AcceleratorTable(std::span<const ACCEL> accelerators);

    bool translate(HWND window, MSG& message) const;

private:
    platform::win32::UniqueAcceleratorTable table_;
};

}
