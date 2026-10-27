#include "ui/inspector/ConfidenceValue.h"

#include <algorithm>
#include <cmath>
#include <format>

#include "platform/win32/Win32Error.h"
#include "ui/SyncBar.h"
#include "ui/framework/BufferedPaint.h"
#include "ui/framework/Dpi.h"
#include "ui/framework/WindowClass.h"

namespace westie::ui {

namespace {

constexpr int BarWidthDips = 110;
constexpr int BarHeightDips = 6;
constexpr int BarGapDips = 6;
constexpr double PercentPerWhole = 100.0;
constexpr UINT LineFormat = DT_SINGLELINE | DT_VCENTER | DT_NOPREFIX | DT_END_ELLIPSIS;

}

ConfidenceValue::ConfidenceValue(HWND parent) : Window(window_class(), {.style = WS_CHILD | WS_VISIBLE, .parent = parent}) {
    set_confidence(0.0);
}

void ConfidenceValue::set_font(HFONT font) {
    font_ = font;
    platform::win32::check(InvalidateRect(hwnd(), nullptr, FALSE), "InvalidateRect");
}

void ConfidenceValue::set_confidence(double confidence) {
    confidence_ = confidence;
    text_ = std::format(L"{}%", std::lround(confidence * PercentPerWhole));
    platform::win32::check(SetWindowTextW(hwnd(), text_.c_str()), "SetWindowTextW");
    platform::win32::check(InvalidateRect(hwnd(), nullptr, FALSE), "InvalidateRect");
}

LRESULT ConfidenceValue::handle_message(UINT message, WPARAM wparam, LPARAM lparam) {
    switch (message) {
    case WM_PAINT: paint(); return 0;
    default:       return Window::handle_message(message, wparam, lparam);
    }
}

const framework::WindowClass& ConfidenceValue::window_class() {
    // The paint fills the background, so the text isn't erased first and doesn't blink
    static framework::WindowClass window_class({.name = L"WestieLib.ConfidenceValue", .style = CS_HREDRAW | CS_VREDRAW});
    return window_class;
}

void ConfidenceValue::paint() const {
    framework::BufferedPaint paint(hwnd());
    RECT client;
    platform::win32::check(GetClientRect(hwnd(), &client), "GetClientRect");
    FillRect(paint.dc(), &client, GetSysColorBrush(COLOR_WINDOW));
    framework::Dpi dpi = framework::Dpi::of(hwnd());

    SelectObject(paint.dc(), font_);
    SIZE text{};
    platform::win32::check(GetTextExtentPoint32W(paint.dc(), text_.c_str(), static_cast<int>(text_.size()), &text), "GetTextExtentPoint32W");
    // A narrow card shortens the bar rather than cutting the percentage
    int bar_width = std::clamp(static_cast<int>(client.right) - dpi.scale(BarGapDips) - static_cast<int>(text.cx), 0, dpi.scale(BarWidthDips));
    int bar_top = (client.bottom - dpi.scale(BarHeightDips)) / 2;
    paint_fraction_bar(paint.dc(), {0, bar_top, bar_width, bar_top + dpi.scale(BarHeightDips)}, confidence_);

    RECT percent{bar_width + dpi.scale(BarGapDips), 0, client.right, client.bottom};
    SetBkMode(paint.dc(), TRANSPARENT);
    SetTextColor(paint.dc(), GetSysColor(COLOR_WINDOWTEXT));
    DrawTextW(paint.dc(), text_.c_str(), static_cast<int>(text_.size()), &percent, LineFormat);
}

}
