#include "ui/RowLook.h"

namespace westie::ui {

namespace {

constexpr COLORREF HotColor = RGB(0xE5, 0xF3, 0xFF);
constexpr COLORREF SelectionColor = RGB(0xCC, 0xE8, 0xFF);
constexpr COLORREF SelectionBorderColor = RGB(0x99, 0xD1, 0xFF);

void paint_box(HDC dc, const framework::Rect& bounds, COLORREF fill, COLORREF border) {
    RECT area = bounds.to_rect();
    auto brush = static_cast<HBRUSH>(GetStockObject(DC_BRUSH));
    SetDCBrushColor(dc, fill);
    FillRect(dc, &area, brush);
    SetDCBrushColor(dc, border);
    FrameRect(dc, &area, brush);
}

}

RowLook row_look(bool selected, bool hot, bool pressed) noexcept {
    if (pressed) {
        return RowLook::Pressed;
    }
    if (selected) {
        return RowLook::Selected;
    }
    return hot ? RowLook::Hot : RowLook::Normal;
}

void paint_row_look(HDC dc, const framework::Rect& bounds, RowLook look) {
    switch (look) {
    case RowLook::Hot:      paint_box(dc, bounds, HotColor, HotColor); return;
    case RowLook::Pressed:
    case RowLook::Selected: paint_box(dc, bounds, SelectionColor, SelectionBorderColor); return;
    case RowLook::Normal:   return;
    }
}

}
