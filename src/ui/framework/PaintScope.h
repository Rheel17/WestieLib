#pragma once

#include <windows.h>

namespace westie::ui::framework {

class PaintScope {
public:
    explicit PaintScope(HWND hwnd) noexcept : hwnd_(hwnd), dc_(BeginPaint(hwnd, &paint_)) {}
    ~PaintScope() { EndPaint(hwnd_, &paint_); }

    PaintScope(const PaintScope&) = delete;
    PaintScope& operator=(const PaintScope&) = delete;

    HDC dc() const noexcept { return dc_; }

private:
    HWND hwnd_;
    PAINTSTRUCT paint_{};
    HDC dc_;
};

}
