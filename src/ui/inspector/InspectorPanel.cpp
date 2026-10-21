#include "ui/inspector/InspectorPanel.h"

#include <algorithm>

#include "platform/win32/Win32Error.h"
#include "ui/framework/Rect.h"

namespace westie::ui {

namespace {

constexpr int SidePaddingDips = 12;
constexpr int TopPaddingDips = 12;
constexpr int BottomPaddingDips = 10;

}

InspectorPanel::InspectorPanel(HWND parent) :
        Panel(parent, L"Inspector"),
        placeholder_(
                hwnd(), L"Add a playlist to see\nits tracks here.",
                {.color = framework::TextColor::Gray, .alignment = framework::LabelAlignment::Center, .wrap = true}
        ) {}

void InspectorPanel::apply_fonts(const framework::FontSet& fonts) {
    placeholder_.set_font(fonts.regular().handle());
    layout();
}

LRESULT InspectorPanel::handle_message(UINT message, WPARAM wparam, LPARAM lparam) {
    switch (message) {
    case WM_SIZE: layout(); return 0;
    default:      return Panel::handle_message(message, wparam, lparam);
    }
}

void InspectorPanel::layout() {
    RECT client;
    platform::win32::check(GetClientRect(hwnd(), &client), "GetClientRect");
    framework::Rect area = framework::Rect::from_rect(client);
    framework::Dpi dpi = framework::Dpi::of(hwnd());
    area.take_left(dpi.scale(SidePaddingDips));
    area.take_right(dpi.scale(SidePaddingDips));
    area.take_top(dpi.scale(TopPaddingDips));
    area.take_bottom(dpi.scale(BottomPaddingDips));

    int height = std::min(placeholder_.height_for(area.width()), area.height());
    int top = area.top + (area.height() - height) / 2;
    platform::win32::check(SetWindowPos(placeholder_.hwnd(), nullptr, area.left, top, area.width(), height, SWP_NOZORDER | SWP_NOACTIVATE), "SetWindowPos");
}

}
