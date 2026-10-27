#include "ui/inspector/LengthValue.h"

#include <format>

#include "platform/win32/Win32Error.h"
#include "ui/DurationText.h"
#include "ui/framework/BufferedPaint.h"
#include "ui/framework/TextColor.h"
#include "ui/framework/WindowClass.h"

namespace westie::ui {

namespace {

constexpr std::chrono::seconds MostOff{5};
constexpr UINT LineFormat = DT_SINGLELINE | DT_VCENTER | DT_NOPREFIX | DT_END_ELLIPSIS;

}

LengthValue::LengthValue(HWND parent) : Window(window_class(), {.style = WS_CHILD | WS_VISIBLE, .parent = parent}) {}

void LengthValue::set_font(HFONT font) {
    font_ = font;
    platform::win32::check(InvalidateRect(hwnd(), nullptr, FALSE), "InvalidateRect");
}

void LengthValue::set_lengths(std::chrono::milliseconds video, std::chrono::milliseconds source) {
    video_ = duration_text(video);
    source_ = std::format(L" · Spotify {}", duration_text(source));
    off_ = video - source > MostOff || source - video > MostOff;
    platform::win32::check(SetWindowTextW(hwnd(), (video_ + source_).c_str()), "SetWindowTextW");
    platform::win32::check(InvalidateRect(hwnd(), nullptr, FALSE), "InvalidateRect");
}

LRESULT LengthValue::handle_message(UINT message, WPARAM wparam, LPARAM lparam) {
    switch (message) {
    case WM_PAINT: paint(); return 0;
    default:       return Window::handle_message(message, wparam, lparam);
    }
}

const framework::WindowClass& LengthValue::window_class() {
    // The paint fills the background, so the text isn't erased first and doesn't blink
    static framework::WindowClass window_class({.name = L"WestieLib.LengthValue", .style = CS_HREDRAW | CS_VREDRAW});
    return window_class;
}

void LengthValue::paint() const {
    framework::BufferedPaint paint(hwnd());
    RECT client;
    platform::win32::check(GetClientRect(hwnd(), &client), "GetClientRect");
    FillRect(paint.dc(), &client, GetSysColorBrush(COLOR_WINDOW));

    SelectObject(paint.dc(), font_);
    SetBkMode(paint.dc(), TRANSPARENT);
    SIZE video{};
    platform::win32::check(GetTextExtentPoint32W(paint.dc(), video_.c_str(), static_cast<int>(video_.size()), &video), "GetTextExtentPoint32W");
    SetTextColor(paint.dc(), framework::text_color(off_ ? framework::TextColor::Warning : framework::TextColor::Normal));
    RECT first = client;
    DrawTextW(paint.dc(), video_.c_str(), static_cast<int>(video_.size()), &first, LineFormat);

    RECT rest{client.left + video.cx, client.top, client.right, client.bottom};
    SetTextColor(paint.dc(), framework::text_color(framework::TextColor::Gray));
    DrawTextW(paint.dc(), source_.c_str(), static_cast<int>(source_.size()), &rest, LineFormat);
}

}
