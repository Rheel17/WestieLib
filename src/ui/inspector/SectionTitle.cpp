#include "ui/inspector/SectionTitle.h"

#include <utility>

#include "platform/win32/Win32Error.h"
#include "ui/framework/BufferedPaint.h"
#include "ui/framework/Dpi.h"
#include "ui/framework/WindowClass.h"

namespace westie::ui {

namespace {

constexpr int RuleGapDips = 8;
// A hairline stays one pixel at every scale
constexpr int HairlinePixels = 1;
constexpr COLORREF RuleColor = RGB(0xDA, 0xDA, 0xDA);
constexpr UINT LineFormat = DT_SINGLELINE | DT_VCENTER | DT_NOPREFIX;

}

SectionTitle::SectionTitle(HWND parent, std::wstring text) :
        Window(window_class(), {.title = text, .style = WS_CHILD | WS_VISIBLE, .parent = parent}), text_(std::move(text)) {}

void SectionTitle::set_font(HFONT font) {
    font_ = font;
    platform::win32::check(InvalidateRect(hwnd(), nullptr, FALSE), "InvalidateRect");
}

LRESULT SectionTitle::handle_message(UINT message, WPARAM wparam, LPARAM lparam) {
    switch (message) {
    case WM_PAINT: paint(); return 0;
    default:       return Window::handle_message(message, wparam, lparam);
    }
}

const framework::WindowClass& SectionTitle::window_class() {
    // The paint fills the background, so the text isn't erased first and doesn't blink
    static framework::WindowClass window_class({.name = L"WestieLib.SectionTitle", .style = CS_HREDRAW | CS_VREDRAW});
    return window_class;
}

void SectionTitle::paint() const {
    framework::BufferedPaint paint(hwnd());
    RECT client;
    platform::win32::check(GetClientRect(hwnd(), &client), "GetClientRect");
    FillRect(paint.dc(), &client, GetSysColorBrush(COLOR_WINDOW));

    SelectObject(paint.dc(), font_);
    SetBkMode(paint.dc(), TRANSPARENT);
    SetTextColor(paint.dc(), GetSysColor(COLOR_WINDOWTEXT));
    RECT text = client;
    DrawTextW(paint.dc(), text_.c_str(), static_cast<int>(text_.size()), &text, LineFormat | DT_CALCRECT);
    text.top = client.top;
    text.bottom = client.bottom;
    DrawTextW(paint.dc(), text_.c_str(), static_cast<int>(text_.size()), &text, LineFormat);

    int rule_top = (client.bottom - HairlinePixels) / 2;
    RECT rule{text.right + framework::Dpi::of(hwnd()).scale(RuleGapDips), rule_top, client.right, rule_top + HairlinePixels};
    if (rule.left < rule.right) {
        SetDCBrushColor(paint.dc(), RuleColor);
        FillRect(paint.dc(), &rule, static_cast<HBRUSH>(GetStockObject(DC_BRUSH)));
    }
}

}
