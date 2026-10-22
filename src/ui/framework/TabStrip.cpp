#include "ui/framework/TabStrip.h"

#include <algorithm>
#include <utility>

#include <windows.h>
#include <commctrl.h>
#include <windowsx.h>

#include "platform/win32/Win32Error.h"
#include "ui/framework/BufferedPaint.h"
#include "ui/framework/SavedDc.h"

namespace westie::ui::framework {

namespace {

constexpr int HeightDips = 22;
constexpr int TextHeightPaddingDips = 6;
constexpr int SidePaddingDips = 8;
constexpr int SeparatorHeightDips = 14;
// A hairline stays one pixel at every scale
constexpr int HairlinePixels = 1;
constexpr COLORREF SelectedColor = RGB(0xCC, 0xE4, 0xF7);
constexpr COLORREF SelectedBorderColor = RGB(0x00, 0x54, 0x99);
constexpr COLORREF HotColor = RGB(0xE5, 0xF3, 0xFF);
constexpr COLORREF HotBorderColor = RGB(0xCC, 0xE8, 0xFF);
constexpr COLORREF SeparatorColor = RGB(0xC9, 0xC9, 0xC9);

void paint_box(HDC dc, const RECT& bounds, COLORREF fill, COLORREF border) {
    auto brush = static_cast<HBRUSH>(GetStockObject(DC_BRUSH));
    SetDCBrushColor(dc, fill);
    FillRect(dc, &bounds, brush);
    SetDCBrushColor(dc, border);
    FrameRect(dc, &bounds, brush);
}

}

TabStrip::TabStrip(HWND parent) :
        Control({
            .class_name = WC_TABCONTROLW,
            .style = WS_CHILD | WS_VISIBLE | WS_TABSTOP | TCS_BUTTONS | TCS_FLATBUTTONS,
            .parent = parent,
        }) {
    // Otherwise a short text gets a wider tab than its padding gives it
    SendMessageW(hwnd(), TCM_SETMINTABWIDTH, 0, 0);
}

void TabStrip::set_tabs(std::span<const std::wstring> texts, int selected) {
    for (int index = 0; std::cmp_less(index, texts.size()); ++index) {
        TCITEMW item{};
        item.mask = TCIF_TEXT;
        // The tab control copies the text and never writes to it
        item.pszText = const_cast<wchar_t*>(texts[index].c_str());
        if (index < count()) {
            platform::win32::check(SendMessageW(hwnd(), TCM_SETITEMW, index, reinterpret_cast<LPARAM>(&item)), "TCM_SETITEMW");
        } else if (SendMessageW(hwnd(), TCM_INSERTITEMW, index, reinterpret_cast<LPARAM>(&item)) == -1) {
            platform::win32::throw_last_error("TCM_INSERTITEMW");
        }
    }
    while (std::cmp_greater(count(), texts.size())) {
        platform::win32::check(SendMessageW(hwnd(), TCM_DELETEITEM, count() - 1, 0), "TCM_DELETEITEM");
    }
    texts_.assign(texts.begin(), texts.end());
    SendMessageW(hwnd(), TCM_SETCURSEL, selected, 0);
    platform::win32::check(InvalidateRect(hwnd(), nullptr, FALSE), "InvalidateRect");
}

int TabStrip::selected() const {
    return static_cast<int>(SendMessageW(hwnd(), TCM_GETCURSEL, 0, 0));
}

int TabStrip::height() const noexcept {
    return height_;
}

void TabStrip::apply_dpi(Dpi dpi, const MessageFont& font) {
    set_font(font.handle());
    font_ = font.handle();
    height_ = std::max(dpi.scale(HeightDips), font.line_height() + dpi.scale(TextHeightPaddingDips));
    SendMessageW(hwnd(), TCM_SETPADDING, 0, MAKELPARAM(dpi.scale(SidePaddingDips), 0));
    SendMessageW(hwnd(), TCM_SETITEMSIZE, 0, MAKELPARAM(0, height_));
}

LRESULT TabStrip::handle_message(UINT message, WPARAM wparam, LPARAM lparam) {
    switch (message) {
    // The control's own buttons are classic 3D ones on a gray band, so the strip paints them as flat toolbar buttons
    case WM_ERASEBKGND:  return TRUE;
    case WM_PAINT:       on_paint(); return 0;
    case WM_PRINTCLIENT: paint(reinterpret_cast<HDC>(wparam)); return 0;
    case WM_MOUSEMOVE:   on_mouse_move(lparam); return Control::handle_message(message, wparam, lparam);
    case WM_MOUSELEAVE:  on_mouse_leave(); return Control::handle_message(message, wparam, lparam);
    default:             return Control::handle_message(message, wparam, lparam);
    }
}

int TabStrip::count() const {
    return static_cast<int>(SendMessageW(hwnd(), TCM_GETITEMCOUNT, 0, 0));
}

RECT TabStrip::item_rect(int index) const {
    RECT rect{};
    platform::win32::check(SendMessageW(hwnd(), TCM_GETITEMRECT, index, reinterpret_cast<LPARAM>(&rect)), "TCM_GETITEMRECT");
    return rect;
}

std::optional<std::size_t> TabStrip::item_at(LPARAM point) const {
    TCHITTESTINFO hit{};
    hit.pt = {GET_X_LPARAM(point), GET_Y_LPARAM(point)};
    auto index = static_cast<int>(SendMessageW(hwnd(), TCM_HITTEST, 0, reinterpret_cast<LPARAM>(&hit)));
    return index >= 0 ? std::optional<std::size_t>(index) : std::nullopt;
}

void TabStrip::paint(HDC dc) const {
    RECT client;
    platform::win32::check(GetClientRect(hwnd(), &client), "GetClientRect");
    FillRect(dc, &client, GetSysColorBrush(COLOR_WINDOW));

    SavedDc saved(dc);
    SelectObject(dc, font_);
    SetBkMode(dc, TRANSPARENT);
    SetTextColor(dc, GetSysColor(COLOR_BTNTEXT));
    int separator_height = Dpi::of(hwnd()).scale(SeparatorHeightDips);
    int selected = this->selected();
    int tabs = std::min(count(), static_cast<int>(texts_.size()));
    for (int index = 0; index < tabs; ++index) {
        RECT item = item_rect(index);
        if (index == selected) {
            paint_box(dc, item, SelectedColor, SelectedBorderColor);
        } else if (tracker_.hot() == static_cast<std::size_t>(index)) {
            paint_box(dc, item, HotColor, HotBorderColor);
        }
        DrawTextW(dc, texts_[index].c_str(), -1, &item, DT_CENTER | DT_VCENTER | DT_SINGLELINE | DT_NOPREFIX);

        if (index + 1 < tabs) {
            RECT next = item_rect(index + 1);
            int left = (item.right + next.left - HairlinePixels) / 2;
            int top = item.top + (item.bottom - item.top - separator_height) / 2;
            RECT line{left, top, left + HairlinePixels, top + separator_height};
            SetDCBrushColor(dc, SeparatorColor);
            FillRect(dc, &line, static_cast<HBRUSH>(GetStockObject(DC_BRUSH)));
        }
    }

    bool focus_hidden = (SendMessageW(hwnd(), WM_QUERYUISTATE, 0, 0) & UISF_HIDEFOCUS) != 0;
    if (GetFocus() == hwnd() && !focus_hidden && selected >= 0 && selected < tabs) {
        RECT focus = item_rect(selected);
        InflateRect(&focus, -2, -2);
        DrawFocusRect(dc, &focus);
    }
}

void TabStrip::on_paint() const {
    BufferedPaint buffer(hwnd());
    paint(buffer.dc());
}

void TabStrip::on_mouse_move(LPARAM point) {
    if (tracker_.move(hwnd(), item_at(point))) {
        platform::win32::check(InvalidateRect(hwnd(), nullptr, FALSE), "InvalidateRect");
    }
}

void TabStrip::on_mouse_leave() {
    if (tracker_.leave()) {
        platform::win32::check(InvalidateRect(hwnd(), nullptr, FALSE), "InvalidateRect");
    }
}

}
