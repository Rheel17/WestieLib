#include "ui/framework/CloseButton.h"

#include <utility>

#include <windows.h>
#include <windowsx.h>

#include "ResourceIds.h"
#include "platform/win32/Win32Error.h"
#include "ui/framework/BufferedPaint.h"
#include "ui/framework/WindowClass.h"

namespace westie::ui::framework {

namespace {

constexpr int BoxDips = 24;
constexpr int IconDips = 12;
constexpr COLORREF HotColor = RGB(0xE5, 0xF3, 0xFF);
constexpr COLORREF PressedColor = RGB(0xCC, 0xE4, 0xF7);

}

CloseButton::CloseButton(HWND parent, COLORREF background, std::function<void()> on_click) :
        Window(window_class(), {.title = L"Close", .style = WS_CHILD | WS_VISIBLE, .parent = parent}),
        background_(background),
        on_click_(std::move(on_click)) {}

int CloseButton::size() const noexcept {
    return size_;
}

void CloseButton::apply_dpi(Dpi dpi) {
    size_ = dpi.scale(BoxDips);
    icon_.emplace(IDI_CLOSE, dpi.scale(IconDips));
    platform::win32::check(InvalidateRect(hwnd(), nullptr, FALSE), "InvalidateRect");
}

LRESULT CloseButton::handle_message(UINT message, WPARAM wparam, LPARAM lparam) {
    switch (message) {
    case WM_MOUSEMOVE:      repaint_if(mouse_.move(hwnd(), item_at(lparam))); return 0;
    case WM_MOUSELEAVE:     repaint_if(mouse_.leave()); return 0;
    case WM_LBUTTONDOWN:    repaint_if(mouse_.press(hwnd(), item_at(lparam))); return 0;
    case WM_LBUTTONUP:      on_left_button_up(lparam); return 0;
    case WM_CAPTURECHANGED: repaint_if(mouse_.cancel()); return 0;
    case WM_PAINT:          paint(); return 0;
    default:                return Window::handle_message(message, wparam, lparam);
    }
}

const WindowClass& CloseButton::window_class() {
    // Without a background brush nothing erases the box before it's painted again, which would make it blink while a
    // splitter moves
    static WindowClass window_class({.name = L"WestieLib.CloseButton", .style = CS_HREDRAW | CS_VREDRAW});
    return window_class;
}

std::optional<std::size_t> CloseButton::item_at(LPARAM point) const {
    RECT client;
    platform::win32::check(GetClientRect(hwnd(), &client), "GetClientRect");
    POINT at{GET_X_LPARAM(point), GET_Y_LPARAM(point)};
    return PtInRect(&client, at) ? std::optional<std::size_t>(0) : std::nullopt;
}

void CloseButton::repaint_if(bool changed) const {
    if (changed) {
        platform::win32::check(InvalidateRect(hwnd(), nullptr, FALSE), "InvalidateRect");
    }
}

void CloseButton::on_left_button_up(LPARAM point) {
    std::optional<std::size_t> clicked = mouse_.release(hwnd(), item_at(point));
    repaint_if(true);
    if (clicked) {
        on_click_();
    }
}

void CloseButton::paint() const {
    BufferedPaint paint(hwnd());
    RECT client;
    platform::win32::check(GetClientRect(hwnd(), &client), "GetClientRect");
    COLORREF fill = mouse_.pressed() ? PressedColor : mouse_.hot() ? HotColor : background_;
    SetDCBrushColor(paint.dc(), fill);
    FillRect(paint.dc(), &client, static_cast<HBRUSH>(GetStockObject(DC_BRUSH)));
    if (icon_) {
        icon_->draw(paint.dc(), (client.right - icon_->size()) / 2, (client.bottom - icon_->size()) / 2);
    }
}

}
