#include "ui/inspector/OutputBox.h"

#include <utility>

#include "platform/win32/UniqueHandle.h"
#include "platform/win32/Win32Error.h"
#include "ui/framework/BufferedPaint.h"
#include "ui/framework/Dpi.h"
#include "ui/framework/WindowClass.h"

namespace westie::ui {

namespace {

constexpr COLORREF BorderColor = RGB(0xC5, 0xC5, 0xC5);
constexpr int BorderDips = 1;
constexpr int PaddingTopDips = 4;
constexpr int PaddingSideDips = 6;
constexpr UINT TextFormat = DT_WORDBREAK | DT_EDITCONTROL | DT_NOPREFIX;

}

OutputBox::OutputBox(HWND parent) : Window(window_class(), {.style = WS_CHILD | WS_VISIBLE, .parent = parent}) {}

void OutputBox::set_font(HFONT font) {
    font_ = font;
    platform::win32::check(InvalidateRect(hwnd(), nullptr, FALSE), "InvalidateRect");
}

void OutputBox::set_text(std::wstring text) {
    if (text == text_) {
        return;
    }
    platform::win32::check(SetWindowTextW(hwnd(), text.c_str()), "SetWindowTextW");
    text_ = std::move(text);
    platform::win32::check(InvalidateRect(hwnd(), nullptr, FALSE), "InvalidateRect");
}

int OutputBox::height_for(int width) const {
    platform::win32::UniqueMemoryDc dc(platform::win32::check(CreateCompatibleDC(nullptr), "CreateCompatibleDC"));
    SelectObject(dc.get(), font_);
    RECT area = text_area({0, 0, width, 0});
    RECT bounds{0, 0, area.right - area.left, 0};
    DrawTextW(dc.get(), text_.c_str(), static_cast<int>(text_.size()), &bounds, TextFormat | DT_CALCRECT);
    framework::Dpi dpi = framework::Dpi::of(hwnd());
    return bounds.bottom + 2 * (dpi.scale(BorderDips) + dpi.scale(PaddingTopDips));
}

LRESULT OutputBox::handle_message(UINT message, WPARAM wparam, LPARAM lparam) {
    switch (message) {
    case WM_PAINT: paint(); return 0;
    default:       return Window::handle_message(message, wparam, lparam);
    }
}

const framework::WindowClass& OutputBox::window_class() {
    // The text wraps, so it moves when the width changes; the paint fills the background instead of an erase that blinks
    static framework::WindowClass window_class({.name = L"WestieLib.OutputBox", .style = CS_HREDRAW | CS_VREDRAW});
    return window_class;
}

RECT OutputBox::text_area(RECT box) const {
    framework::Dpi dpi = framework::Dpi::of(hwnd());
    int side = dpi.scale(BorderDips) + dpi.scale(PaddingSideDips);
    int top = dpi.scale(BorderDips) + dpi.scale(PaddingTopDips);
    return {box.left + side, box.top + top, box.right - side, box.bottom - top};
}

void OutputBox::paint() const {
    framework::BufferedPaint paint(hwnd());
    RECT client;
    platform::win32::check(GetClientRect(hwnd(), &client), "GetClientRect");
    SetDCBrushColor(paint.dc(), BorderColor);
    FillRect(paint.dc(), &client, static_cast<HBRUSH>(GetStockObject(DC_BRUSH)));
    int border = framework::Dpi::of(hwnd()).scale(BorderDips);
    RECT inside{client.left + border, client.top + border, client.right - border, client.bottom - border};
    FillRect(paint.dc(), &inside, GetSysColorBrush(COLOR_WINDOW));

    SelectObject(paint.dc(), font_);
    SetBkMode(paint.dc(), TRANSPARENT);
    SetTextColor(paint.dc(), GetSysColor(COLOR_WINDOWTEXT));
    RECT area = text_area(client);
    DrawTextW(paint.dc(), text_.c_str(), static_cast<int>(text_.size()), &area, TextFormat);
}

}
