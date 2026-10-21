#include "ui/framework/Label.h"

#include <utility>

#include "platform/win32/UniqueHandle.h"
#include "platform/win32/Win32Error.h"
#include "ui/framework/BufferedPaint.h"
#include "ui/framework/WindowClass.h"

namespace westie::ui::framework {

Label::Label(HWND parent, std::wstring text, const LabelStyle& style) :
        Window(window_class(), {.title = text, .style = WS_CHILD | WS_VISIBLE, .parent = parent}), text_(std::move(text)), style_(style) {}

void Label::set_font(HFONT font) {
    font_ = font;
    platform::win32::check(InvalidateRect(hwnd(), nullptr, TRUE), "InvalidateRect");
}

int Label::height_for(int width) const {
    platform::win32::UniqueMemoryDc dc(platform::win32::check(CreateCompatibleDC(nullptr), "CreateCompatibleDC"));
    SelectObject(dc.get(), font_);
    RECT bounds{0, 0, width, 0};
    DrawTextW(dc.get(), text_.c_str(), static_cast<int>(text_.size()), &bounds, format() | DT_CALCRECT);
    return bounds.bottom;
}

LRESULT Label::handle_message(UINT message, WPARAM wparam, LPARAM lparam) {
    switch (message) {
    case WM_PAINT: paint(); return 0;
    default:       return Window::handle_message(message, wparam, lparam);
    }
}

const WindowClass& Label::window_class() {
    // Centered and wrapped text moves when the width changes. Without a background brush nothing erases the text before
    // it's painted again, which would make it blink while a splitter moves.
    static WindowClass window_class({.name = L"WestieLib.Label", .style = CS_HREDRAW | CS_VREDRAW});
    return window_class;
}

UINT Label::format() const noexcept {
    UINT lines = style_.wrap ? DT_WORDBREAK : DT_SINGLELINE | DT_END_ELLIPSIS;
    return DT_NOPREFIX | lines | (style_.alignment == LabelAlignment::Center ? DT_CENTER : DT_LEFT);
}

void Label::paint() const {
    BufferedPaint paint(hwnd());
    RECT client;
    platform::win32::check(GetClientRect(hwnd(), &client), "GetClientRect");
    FillRect(paint.dc(), &client, GetSysColorBrush(COLOR_WINDOW));
    SelectObject(paint.dc(), font_);
    SetBkMode(paint.dc(), TRANSPARENT);
    SetTextColor(paint.dc(), GetSysColor(style_.color == TextColor::Gray ? COLOR_GRAYTEXT : COLOR_WINDOWTEXT));
    DrawTextW(paint.dc(), text_.c_str(), static_cast<int>(text_.size()), &client, format());
}

}
