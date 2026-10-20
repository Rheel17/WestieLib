#pragma once

#include <windows.h>

#include "platform/win32/UniqueHandle.h"
#include "ui/framework/Dpi.h"

namespace westie::ui::framework {

class WindowIcon {
public:
    explicit WindowIcon(int resource_id) noexcept;

    void apply_to(HWND hwnd, Dpi dpi);

private:
    platform::win32::UniqueIcon load(int size_metric, Dpi dpi) const;

    int resource_id_;
    platform::win32::UniqueIcon big_;
    platform::win32::UniqueIcon small_;
};

}
