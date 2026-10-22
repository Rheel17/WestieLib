#include "ui/framework/IconView.h"

#include "platform/win32/Win32Error.h"
#include "ui/framework/BufferedPaint.h"
#include "ui/framework/WindowClass.h"

namespace westie::ui::framework {

IconView::IconView(HWND parent, int resource_id, int size_dips) :
        Window(window_class(), {.style = WS_CHILD | WS_VISIBLE, .parent = parent}),
        resource_id_(resource_id),
        size_dips_(size_dips),
        icon_(resource_id, Dpi::of(hwnd()).scale(size_dips)) {}

int IconView::size() const noexcept {
    return icon_.size();
}

void IconView::apply_dpi(Dpi dpi) {
    icon_ = IconResource(resource_id_, dpi.scale(size_dips_));
    platform::win32::check(InvalidateRect(hwnd(), nullptr, FALSE), "InvalidateRect");
}

LRESULT IconView::handle_message(UINT message, WPARAM wparam, LPARAM lparam) {
    switch (message) {
    case WM_PAINT: paint(); return 0;
    // A click goes to the window the icon sits in, as if the icon were painted there
    case WM_NCHITTEST: return HTTRANSPARENT;
    default:           return Window::handle_message(message, wparam, lparam);
    }
}

const WindowClass& IconView::window_class() {
    // The paint fills the background, so the icon isn't erased first and doesn't blink
    static WindowClass window_class({.name = L"WestieLib.IconView", .style = CS_HREDRAW | CS_VREDRAW});
    return window_class;
}

void IconView::paint() const {
    BufferedPaint paint(hwnd());
    RECT client;
    platform::win32::check(GetClientRect(hwnd(), &client), "GetClientRect");
    FillRect(paint.dc(), &client, GetSysColorBrush(COLOR_WINDOW));
    icon_.draw(paint.dc(), (client.right - icon_.size()) / 2, (client.bottom - icon_.size()) / 2);
}

}
