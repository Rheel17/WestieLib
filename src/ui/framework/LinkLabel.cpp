#include "ui/framework/LinkLabel.h"

#include <algorithm>
#include <string_view>
#include <utility>

#include <windows.h>
#include <windowsx.h>

#include "platform/win32/Win32Error.h"
#include "ui/framework/BufferedPaint.h"
#include "ui/framework/WindowClass.h"

namespace westie::ui::framework {

namespace {

constexpr COLORREF LinkColor = RGB(0x00, 0x66, 0xCC);
constexpr std::wstring_view Separator = L", ";
constexpr UINT LineFormat = DT_SINGLELINE | DT_NOPREFIX | DT_END_ELLIPSIS;

POINT point_of(LPARAM lparam) {
    return {GET_X_LPARAM(lparam), GET_Y_LPARAM(lparam)};
}

int text_width(HDC dc, std::wstring_view text) {
    SIZE extent{};
    platform::win32::check(GetTextExtentPoint32W(dc, text.data(), static_cast<int>(text.size()), &extent), "GetTextExtentPoint32W");
    return extent.cx;
}

}

LinkLabel::LinkLabel(HWND parent, std::function<void(std::size_t)> on_click) :
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
    set_links({std::move(text)});
}

void LinkLabel::set_links(std::vector<std::wstring> links) {
    if (links == links_) {
        return;
    }
    std::wstring text;
    for (const std::wstring& link : links) {
        if (!text.empty()) {
            text += Separator;
        }
        text += link;
    }
    platform::win32::check(SetWindowTextW(hwnd(), text.c_str()), "SetWindowTextW");
    links_ = std::move(links);
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

std::vector<LinkLabel::Span> LinkLabel::spans() const {
    RECT client;
    platform::win32::check(GetClientRect(hwnd(), &client), "GetClientRect");
    platform::win32::UniqueMemoryDc dc(platform::win32::check(CreateCompatibleDC(nullptr), "CreateCompatibleDC"));
    SelectObject(dc.get(), font_);
    int separator = text_width(dc.get(), Separator);

    std::vector<Span> found;
    int left = 0;
    for (const std::wstring& link : links_) {
        if (left >= client.right) {
            break;
        }
        int right = left + text_width(dc.get(), link);
        found.push_back({.left = left, .right = std::min(right, static_cast<int>(client.right))});
        left = right + separator;
    }
    return found;
}

std::optional<std::size_t> LinkLabel::item_at(POINT point) const {
    // Only the links are links, not the separators or the room after them
    RECT client;
    platform::win32::check(GetClientRect(hwnd(), &client), "GetClientRect");
    if (point.y < client.top || point.y >= client.bottom) {
        return std::nullopt;
    }
    std::vector<Span> found = spans();
    auto hit = std::ranges::find_if(found, [&](const Span& span) { return point.x >= span.left && point.x < span.right; });
    if (hit == found.end()) {
        return std::nullopt;
    }
    return static_cast<std::size_t>(hit - found.begin());
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
        on_click_(*clicked);
    }
}

void LinkLabel::paint() const {
    BufferedPaint paint(hwnd());
    RECT client;
    platform::win32::check(GetClientRect(hwnd(), &client), "GetClientRect");
    FillRect(paint.dc(), &client, GetSysColorBrush(COLOR_WINDOW));
    SetBkMode(paint.dc(), TRANSPARENT);

    std::vector<Span> found = spans();
    for (std::size_t index = 0; index < found.size(); ++index) {
        const std::wstring& link = links_[index];
        SelectObject(paint.dc(), mouse_.hot() == index ? underlined_.get() : font_);
        SetTextColor(paint.dc(), LinkColor);
        RECT text{found[index].left, client.top, found[index].right, client.bottom};
        DrawTextW(paint.dc(), link.c_str(), static_cast<int>(link.size()), &text, LineFormat);
        if (index + 1 < found.size()) {
            SelectObject(paint.dc(), font_);
            SetTextColor(paint.dc(), GetSysColor(COLOR_WINDOWTEXT));
            RECT separator{found[index].right, client.top, found[index + 1].left, client.bottom};
            DrawTextW(paint.dc(), Separator.data(), static_cast<int>(Separator.size()), &separator, LineFormat);
        }
    }
}

}
