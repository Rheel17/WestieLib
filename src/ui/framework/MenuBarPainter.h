#pragma once

#include <windows.h>

#include "platform/win32/UniqueHandle.h"

namespace westie::ui::framework {

class MenuBarPainter {
public:
    MenuBarPainter(HWND window, HBRUSH background) noexcept;

    void load_theme();
    bool draw(UINT message, LPARAM lparam) const;
    void cover_edge() const;

private:
    RECT bar_rect() const;
    void draw_background(HDC dc) const;
    void draw_item(HDC dc, HMENU menu, int position, const DRAWITEMSTRUCT& item) const;

    HWND window_;
    HBRUSH background_;
    platform::win32::UniqueTheme theme_;
};

}
