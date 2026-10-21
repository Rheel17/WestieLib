#pragma once

#include <windows.h>

namespace westie::ui::framework {

class WindowDc {
public:
    explicit WindowDc(HWND hwnd) noexcept : hwnd_(hwnd), dc_(GetWindowDC(hwnd)) {}
    ~WindowDc() { ReleaseDC(hwnd_, dc_); }

    WindowDc(const WindowDc&) = delete;
    WindowDc& operator=(const WindowDc&) = delete;

    HDC get() const noexcept { return dc_; }

private:
    HWND hwnd_;
    HDC dc_;
};

}
