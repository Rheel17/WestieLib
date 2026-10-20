#pragma once

#include <windows.h>

#include "ui/framework/Size.h"

namespace westie::ui::framework {

class Dpi {
public:
    constexpr static UINT Standard = USER_DEFAULT_SCREEN_DPI;

    constexpr explicit Dpi(UINT value) noexcept : value_(value) {}

    static Dpi of(HWND hwnd) noexcept { return Dpi(GetDpiForWindow(hwnd)); }

    constexpr UINT value() const noexcept { return value_; }

    int scale(int dips) const noexcept { return MulDiv(dips, static_cast<int>(value_), static_cast<int>(Standard)); }

    Size scale(Size dips) const noexcept { return {scale(dips.width), scale(dips.height)}; }

    int unscale(int pixels) const noexcept { return MulDiv(pixels, static_cast<int>(Standard), static_cast<int>(value_)); }

private:
    UINT value_;
};

}
