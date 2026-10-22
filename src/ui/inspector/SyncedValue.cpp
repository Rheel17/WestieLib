#include "ui/inspector/SyncedValue.h"

#include <algorithm>
#include <format>

#include "platform/win32/Win32Error.h"
#include "ui/SyncBar.h"
#include "ui/framework/BufferedPaint.h"
#include "ui/framework/Dpi.h"
#include "ui/framework/Rect.h"
#include "ui/framework/WindowClass.h"

namespace westie::ui {

namespace {

constexpr int BarHeightDips = 6;
constexpr int BarGapDips = 6;
constexpr UINT LineFormat = DT_SINGLELINE | DT_VCENTER | DT_NOPREFIX | DT_END_ELLIPSIS;

}

SyncedValue::SyncedValue(HWND parent) : Window(window_class(), {.style = WS_CHILD | WS_VISIBLE, .parent = parent}) {
    set_counts({});
}

void SyncedValue::set_font(HFONT font) {
    font_ = font;
    platform::win32::check(InvalidateRect(hwnd(), nullptr, FALSE), "InvalidateRect");
}

void SyncedValue::set_counts(const TrackCounts& counts) {
    counts_ = counts;
    text_ = std::format(L"{}/{}", counts.synced, counts.total);
    platform::win32::check(SetWindowTextW(hwnd(), text_.c_str()), "SetWindowTextW");
    platform::win32::check(InvalidateRect(hwnd(), nullptr, FALSE), "InvalidateRect");
}

LRESULT SyncedValue::handle_message(UINT message, WPARAM wparam, LPARAM lparam) {
    switch (message) {
    case WM_PAINT: paint(); return 0;
    default:       return Window::handle_message(message, wparam, lparam);
    }
}

const framework::WindowClass& SyncedValue::window_class() {
    // The paint fills the background, so the text isn't erased first and doesn't blink
    static framework::WindowClass window_class({.name = L"WestieLib.SyncedValue", .style = CS_HREDRAW | CS_VREDRAW});
    return window_class;
}

void SyncedValue::paint() const {
    framework::BufferedPaint paint(hwnd());
    RECT client;
    platform::win32::check(GetClientRect(hwnd(), &client), "GetClientRect");
    FillRect(paint.dc(), &client, GetSysColorBrush(COLOR_WINDOW));
    framework::Dpi dpi = framework::Dpi::of(hwnd());

    SelectObject(paint.dc(), font_);
    SIZE text{};
    platform::win32::check(GetTextExtentPoint32W(paint.dc(), text_.c_str(), static_cast<int>(text_.size()), &text), "GetTextExtentPoint32W");
    int bar_width = std::max(0, static_cast<int>(client.right) - dpi.scale(BarGapDips) - static_cast<int>(text.cx));
    int bar_top = (client.bottom - dpi.scale(BarHeightDips)) / 2;
    paint_sync_bar(paint.dc(), {0, bar_top, bar_width, bar_top + dpi.scale(BarHeightDips)}, counts_);

    RECT counts{bar_width + dpi.scale(BarGapDips), 0, client.right, client.bottom};
    SetBkMode(paint.dc(), TRANSPARENT);
    SetTextColor(paint.dc(), GetSysColor(COLOR_WINDOWTEXT));
    DrawTextW(paint.dc(), text_.c_str(), static_cast<int>(text_.size()), &counts, LineFormat);
}

}
