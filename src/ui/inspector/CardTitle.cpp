#include "ui/inspector/CardTitle.h"

#include <algorithm>
#include <utility>

#include "platform/win32/Win32Error.h"
#include "ui/framework/BufferedPaint.h"
#include "ui/framework/WindowClass.h"

namespace westie::ui {

namespace {

constexpr int IconDips = 16;
constexpr int IconGapDips = 7;
constexpr int LineDips = 20;
constexpr UINT LineFormat = DT_SINGLELINE | DT_VCENTER | DT_NOPREFIX | DT_END_ELLIPSIS;

}

CardTitle::CardTitle(HWND parent) : Window(window_class(), {.style = WS_CHILD | WS_VISIBLE, .parent = parent}) {}

int CardTitle::height() const noexcept {
    return height_;
}

int CardTitle::text_left() const {
    framework::Dpi dpi = framework::Dpi::of(hwnd());
    return dpi.scale(IconDips) + dpi.scale(IconGapDips);
}

void CardTitle::apply_dpi(framework::Dpi dpi, const framework::FontSet& fonts) {
    font_ = fonts.title().handle();
    height_ = std::max(dpi.scale(LineDips), fonts.title().line_height());
    load_icon(dpi);
    platform::win32::check(InvalidateRect(hwnd(), nullptr, FALSE), "InvalidateRect");
}

void CardTitle::set(int icon, std::wstring text) {
    if (icon != icon_id_) {
        icon_id_ = icon;
        load_icon(framework::Dpi::of(hwnd()));
    }
    if (text != text_) {
        platform::win32::check(SetWindowTextW(hwnd(), text.c_str()), "SetWindowTextW");
        text_ = std::move(text);
    }
    platform::win32::check(InvalidateRect(hwnd(), nullptr, FALSE), "InvalidateRect");
}

LRESULT CardTitle::handle_message(UINT message, WPARAM wparam, LPARAM lparam) {
    switch (message) {
    case WM_PAINT: paint(); return 0;
    default:       return Window::handle_message(message, wparam, lparam);
    }
}

const framework::WindowClass& CardTitle::window_class() {
    // The paint fills the background, so the text isn't erased first and doesn't blink
    static framework::WindowClass window_class({.name = L"WestieLib.CardTitle", .style = CS_HREDRAW | CS_VREDRAW});
    return window_class;
}

void CardTitle::load_icon(framework::Dpi dpi) {
    if (icon_id_ != 0) {
        icon_.emplace(icon_id_, dpi.scale(IconDips));
    }
}

void CardTitle::paint() const {
    framework::BufferedPaint paint(hwnd());
    RECT client;
    platform::win32::check(GetClientRect(hwnd(), &client), "GetClientRect");
    FillRect(paint.dc(), &client, GetSysColorBrush(COLOR_WINDOW));
    if (icon_) {
        icon_->draw(paint.dc(), 0, (client.bottom - icon_->size()) / 2);
    }

    RECT text{text_left(), 0, client.right, client.bottom};
    SelectObject(paint.dc(), font_);
    SetBkMode(paint.dc(), TRANSPARENT);
    SetTextColor(paint.dc(), GetSysColor(COLOR_WINDOWTEXT));
    DrawTextW(paint.dc(), text_.c_str(), static_cast<int>(text_.size()), &text, LineFormat);
}

}
