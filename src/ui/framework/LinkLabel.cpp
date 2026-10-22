#include "ui/framework/LinkLabel.h"

#include <algorithm>
#include <utility>

#include <windows.h>
#include <windowsx.h>

#include "platform/win32/Win32Error.h"
#include "ui/framework/BufferedPaint.h"
#include "ui/framework/WindowClass.h"

namespace westie::ui::framework {

namespace {

constexpr COLORREF LinkColor = RGB(0x00, 0x66, 0xCC);
constexpr UINT LineFormat = DT_SINGLELINE | DT_NOPREFIX | DT_END_ELLIPSIS;

POINT point_of(LPARAM lparam) {
    return {GET_X_LPARAM(lparam), GET_Y_LPARAM(lparam)};
}

}

LinkLabel::LinkLabel(HWND parent, std::function<void()> on_click) :
        Window(window_class(), {.style = WS_CHILD | WS_VISIBLE, .parent = parent}), on_click_(std::move(on_click)) {}

void LinkLabel::set_font(HFONT font) {
    LOGFONTW description{};
    platform::win32::check(GetObjectW(font, sizeof(description), &description), "GetObjectW");
    description.lfUnderline = TRUE;
    underlined_.reset(platform::win32::check(CreateFontIndirectW(&description), "CreateFontIndirectW"));
    font_ = font;
    platform::win32::check(InvalidateRect(hwnd(), nullptr, FALSE), "InvalidateRect");
}

void LinkLabel::set_text(std::wstring text) {
    if (text == text_) {
        return;
    }
    platform::win32::check(SetWindowTextW(hwnd(), text.c_str()), "SetWindowTextW");
    text_ = std::move(text);
    platform::win32::check(InvalidateRect(hwnd(), nullptr, FALSE), "InvalidateRect");
}

LRESULT LinkLabel::handle_message(UINT message, WPARAM wparam, LPARAM lparam) {
    switch (message) {
    case WM_SETCURSOR:      return on_set_cursor() ? TRUE : Window::handle_message(message, wparam, lparam);
    case WM_MOUSEMOVE:      repaint_if(mouse_.move(hwnd(), item_at(point_of(lparam)))); return 0;
    case WM_MOUSELEAVE:     repaint_if(mouse_.leave()); return 0;
    case WM_LBUTTONDOWN:    repaint_if(mouse_.press(hwnd(), item_at(point_of(lparam)))); return 0;
    case WM_LBUTTONUP:      on_left_button_up(lparam); return 0;
    case WM_CAPTURECHANGED: repaint_if(mouse_.cancel()); return 0;
    case WM_PAINT:          paint(); return 0;
    default:                return Window::handle_message(message, wparam, lparam);
    }
}

const WindowClass& LinkLabel::window_class() {
    // Without a background brush nothing erases the text before it's painted again, which would make it blink while a
    // splitter moves
    static WindowClass window_class({.name = L"WestieLib.LinkLabel", .style = CS_HREDRAW | CS_VREDRAW});
    return window_class;
}

int LinkLabel::text_width() const {
    platform::win32::UniqueMemoryDc dc(platform::win32::check(CreateCompatibleDC(nullptr), "CreateCompatibleDC"));
    SelectObject(dc.get(), font_);
    SIZE extent{};
    platform::win32::check(GetTextExtentPoint32W(dc.get(), text_.c_str(), static_cast<int>(text_.size()), &extent), "GetTextExtentPoint32W");
    return extent.cx;
}

std::optional<std::size_t> LinkLabel::item_at(POINT point) const {
    // Only the text is the link, not the room after it
    RECT client;
    platform::win32::check(GetClientRect(hwnd(), &client), "GetClientRect");
    client.right = std::min(client.right, static_cast<LONG>(text_width()));
    return PtInRect(&client, point) ? std::optional<std::size_t>(0) : std::nullopt;
}

void LinkLabel::repaint_if(bool changed) const {
    if (changed) {
        platform::win32::check(InvalidateRect(hwnd(), nullptr, FALSE), "InvalidateRect");
    }
}

bool LinkLabel::on_set_cursor() const {
    POINT point;
    if (!GetCursorPos(&point) || !ScreenToClient(hwnd(), &point) || !item_at(point)) {
        return false;
    }
    SetCursor(LoadCursorW(nullptr, IDC_HAND));
    return true;
}

void LinkLabel::on_left_button_up(LPARAM lparam) {
    std::optional<std::size_t> clicked = mouse_.release(hwnd(), item_at(point_of(lparam)));
    repaint_if(true);
    if (clicked) {
        on_click_();
    }
}

void LinkLabel::paint() const {
    BufferedPaint paint(hwnd());
    RECT client;
    platform::win32::check(GetClientRect(hwnd(), &client), "GetClientRect");
    FillRect(paint.dc(), &client, GetSysColorBrush(COLOR_WINDOW));
    SelectObject(paint.dc(), mouse_.hot() ? underlined_.get() : font_);
    SetBkMode(paint.dc(), TRANSPARENT);
    SetTextColor(paint.dc(), LinkColor);
    DrawTextW(paint.dc(), text_.c_str(), static_cast<int>(text_.size()), &client, LineFormat);
}

}
