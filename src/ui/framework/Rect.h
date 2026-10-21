#pragma once

#include <algorithm>

#include <windows.h>

namespace westie::ui::framework {

struct Rect {
    int left = 0;
    int top = 0;
    int right = 0;
    int bottom = 0;

    static Rect from_rect(const RECT& rect) {
        return {static_cast<int>(rect.left), static_cast<int>(rect.top), static_cast<int>(rect.right), static_cast<int>(rect.bottom)};
    }

    RECT to_rect() const { return {left, top, right, bottom}; }

    bool operator==(const Rect&) const = default;

    int width() const { return right - left; }
    int height() const { return bottom - top; }

    Rect take_left(int width) {
        Rect taken{left, top, std::clamp(left + width, left, right), bottom};
        left = taken.right;
        return taken;
    }

    Rect take_right(int width) {
        Rect taken{std::clamp(right - width, left, right), top, right, bottom};
        right = taken.left;
        return taken;
    }

    Rect take_top(int height) {
        Rect taken{left, top, right, std::clamp(top + height, top, bottom)};
        top = taken.bottom;
        return taken;
    }

    Rect take_bottom(int height) {
        Rect taken{left, std::clamp(bottom - height, top, bottom), right, bottom};
        bottom = taken.top;
        return taken;
    }
};

}
