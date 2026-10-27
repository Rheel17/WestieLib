#include "ui/inspector/SourceLink.h"

#include <cstddef>
#include <format>
#include <utility>

#include "platform/win32/Win32Error.h"
#include "ui/SourceLogo.h"
#include "ui/framework/BufferedPaint.h"
#include "ui/framework/DeferredPositions.h"
#include "ui/framework/Rect.h"
#include "ui/framework/WindowClass.h"

namespace westie::ui {

namespace {

constexpr IconTextStyle NameStyle{.gap_dips = 6, .line_dips = 16};

const wchar_t* source_name(core::SourceType type) {
    switch (type) {
    case core::SourceType::YouTubeMusic: return L"YouTube Music";
    case core::SourceType::Spotify:      return L"Spotify";
    }
    return L"";
}

const wchar_t* open_text(core::SourceType type) {
    return type == core::SourceType::Spotify ? L"Open track" : L"Open";
}

}

SourceLink::SourceLink(HWND parent, std::function<void()> on_click) :
        Window(window_class(), {.style = WS_CHILD | WS_VISIBLE | WS_CLIPCHILDREN, .parent = parent}),
        name_(hwnd(), NameStyle),
        link_(hwnd(), [on_click = std::move(on_click)](std::size_t) { on_click(); }) {}

void SourceLink::apply_dpi(framework::Dpi dpi, const framework::MessageFont& font) {
    name_.apply_dpi(dpi, font);
    link_.set_font(font.handle());
    layout();
}

void SourceLink::set_source(core::SourceType type) {
    std::wstring name = std::format(L"{} · ", source_name(type));
    platform::win32::check(SetWindowTextW(hwnd(), std::format(L"{}{}", name, open_text(type)).c_str()), "SetWindowTextW");
    name_.set(source_logo_icon(type), std::move(name));
    link_.set_text(open_text(type));
    layout();
}

LRESULT SourceLink::handle_message(UINT message, WPARAM wparam, LPARAM lparam) {
    switch (message) {
    case WM_SIZE:  layout(); return 0;
    case WM_PAINT: paint(); return 0;
    default:       return Window::handle_message(message, wparam, lparam);
    }
}

const framework::WindowClass& SourceLink::window_class() {
    // The paint fills the background, so the children's text isn't erased first and doesn't blink
    static framework::WindowClass window_class({.name = L"WestieLib.SourceLink", .style = CS_HREDRAW | CS_VREDRAW});
    return window_class;
}

void SourceLink::layout() {
    RECT client;
    platform::win32::check(GetClientRect(hwnd(), &client), "GetClientRect");
    framework::Rect area = framework::Rect::from_rect(client);
    framework::DeferredPositions positions;
    positions.place(name_.hwnd(), area.take_left(name_.width()));
    positions.place(link_.hwnd(), area);
    positions.apply();
}

void SourceLink::paint() const {
    framework::BufferedPaint paint(hwnd());
    RECT client;
    platform::win32::check(GetClientRect(hwnd(), &client), "GetClientRect");
    FillRect(paint.dc(), &client, GetSysColorBrush(COLOR_WINDOW));
}

}
