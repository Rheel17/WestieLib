#pragma once

#include <windows.h>

#include "ui/framework/Rect.h"
#include "ui/framework/Size.h"

namespace westie::ui {

struct Placement {
    framework::Rect normal_bounds{};
    bool maximized = false;
};

class WindowPlacement {
public:
    explicit WindowPlacement(HWND window) noexcept;

    void restore(const Placement& placement);
    void apply_initial_size(framework::Size dips);
    Placement current() const;
    int show_command(int requested) const noexcept;
    bool restoring() const noexcept;

private:
    HWND window_;
    bool start_maximized_ = false;
    bool restoring_ = false;
};

}
