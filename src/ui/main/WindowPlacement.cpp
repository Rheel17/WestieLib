#include "ui/main/WindowPlacement.h"

#include <algorithm>

#include "platform/win32/Win32Error.h"
#include "ui/framework/Dpi.h"

namespace westie::ui {

WindowPlacement::WindowPlacement(HWND window) noexcept : window_(window) {}

void WindowPlacement::restore(const Placement& placement) {
    WINDOWPLACEMENT values{};
    values.length = sizeof(values);
    values.showCmd = SW_HIDE;
    values.rcNormalPosition = placement.normal_bounds.to_rect();

    start_maximized_ = placement.maximized;
    restoring_ = true;
    BOOL restored = SetWindowPlacement(window_, &values);
    restoring_ = false;
    platform::win32::check(restored, "SetWindowPlacement");
}

void WindowPlacement::apply_initial_size(framework::Size dips) {
    framework::Size size = framework::Dpi::of(window_).scale(dips);

    MONITORINFO monitor{};
    monitor.cbSize = sizeof(monitor);
    platform::win32::check(GetMonitorInfoW(MonitorFromWindow(window_, MONITOR_DEFAULTTONEAREST), &monitor), "GetMonitorInfoW");
    const RECT& work = monitor.rcWork;
    size.width = std::min(size.width, static_cast<int>(work.right - work.left));
    size.height = std::min(size.height, static_cast<int>(work.bottom - work.top));

    RECT window;
    platform::win32::check(GetWindowRect(window_, &window), "GetWindowRect");
    int x = std::clamp(static_cast<int>(window.left), static_cast<int>(work.left), static_cast<int>(work.right) - size.width);
    int y = std::clamp(static_cast<int>(window.top), static_cast<int>(work.top), static_cast<int>(work.bottom) - size.height);
    platform::win32::check(SetWindowPos(window_, nullptr, x, y, size.width, size.height, SWP_NOZORDER | SWP_NOACTIVATE), "SetWindowPos");
}

Placement WindowPlacement::current() const {
    WINDOWPLACEMENT values{};
    values.length = sizeof(values);
    platform::win32::check(GetWindowPlacement(window_, &values), "GetWindowPlacement");

    bool minimized_from_maximized = values.showCmd == SW_SHOWMINIMIZED && (values.flags & WPF_RESTORETOMAXIMIZED);
    return {
        .normal_bounds = framework::Rect::from_rect(values.rcNormalPosition),
        .maximized = values.showCmd == SW_SHOWMAXIMIZED || minimized_from_maximized,
    };
}

int WindowPlacement::show_command(int requested) const noexcept {
    bool default_show = requested == SW_SHOWNORMAL || requested == SW_SHOWDEFAULT;
    return start_maximized_ && default_show ? SW_SHOWMAXIMIZED : requested;
}

bool WindowPlacement::restoring() const noexcept {
    return restoring_;
}

}
