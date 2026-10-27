#include "ui/inspector/IconText.h"

#include <algorithm>
#include <utility>

#include "platform/win32/UniqueHandle.h"
#include "platform/win32/Win32Error.h"
#include "ui/framework/BufferedPaint.h"
#include "ui/framework/WindowClass.h"

namespace westie::ui {

namespace {

constexpr int IconDips = 16;
constexpr UINT LineFormat = DT_SINGLELINE | DT_VCENTER | DT_NOPREFIX | DT_END_ELLIPSIS;

}

IconText::IconText(HWND parent, const IconTextStyle& style) :
        Window(window_class(), {.style = WS_CHILD | WS_VISIBLE, .parent = parent}), style_(style) {}

int IconText::height() const noexcept {
    return height_;
}

int IconText::text_left() const {
    if (icon_id_ == 0) {
        return 0;
    }
    framework::Dpi dpi = framework::Dpi::of(hwnd());
    return dpi.scale(IconDips) + dpi.scale(style_.gap_dips);
}

int IconText::width() const {
    platform::win32::UniqueMemoryDc dc(platform::win32::check(CreateCompatibleDC(nullptr), "CreateCompatibleDC"));
    SelectObject(dc.get(), font_);
    SIZE extent{};
    platform::win32::check(GetTextExtentPoint32W(dc.get(), text_.c_str(), static_cast<int>(text_.size()), &extent), "GetTextExtentPoint32W");
    return text_left() + extent.cx;
}

void IconText::apply_dpi(framework::Dpi dpi, const framework::MessageFont& font) {
    font_ = font.handle();
    height_ = std::max(dpi.scale(style_.line_dips), font.line_height());
    load_icon(dpi);
    platform::win32::check(InvalidateRect(hwnd(), nullptr, FALSE), "InvalidateRect");
}

void IconText::set(int icon, std::wstring text) {
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

LRESULT IconText::handle_message(UINT message, WPARAM wparam, LPARAM lparam) {
    switch (message) {
    case WM_PAINT: paint(); return 0;
    default:       return Window::handle_message(message, wparam, lparam);
    }
}

const framework::WindowClass& IconText::window_class() {
    // The paint fills the background, so the text isn't erased first and doesn't blink
    static framework::WindowClass window_class({.name = L"WestieLib.IconText", .style = CS_HREDRAW | CS_VREDRAW});
    return window_class;
}

void IconText::load_icon(framework::Dpi dpi) {
    if (icon_id_ != 0) {
        icon_.emplace(icon_id_, dpi.scale(IconDips));
    } else {
        icon_.reset();
    }
}

void IconText::paint() const {
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
