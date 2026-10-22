#include "ui/inspector/InspectorPanel.h"

#include <algorithm>
#include <optional>
#include <utility>

#include "platform/win32/Win32Error.h"
#include "ui/framework/DeferredPositions.h"
#include "ui/framework/Rect.h"

namespace westie::ui {

namespace {

constexpr int SidePaddingDips = 12;
constexpr int TopPaddingDips = 12;
constexpr int BottomPaddingDips = 10;

}

InspectorPanel::InspectorPanel(HWND parent, const std::function<void(CommandId)>& on_command) :
        Panel(parent, L"Inspector", WS_VSCROLL),
        placeholder_(
                hwnd(), L"Add a playlist to see\nits tracks here.",
                {.color = framework::TextColor::Gray, .alignment = framework::LabelAlignment::Center, .wrap = true}
        ),
        library_(hwnd(), on_command),
        playlist_(hwnd(), on_command) {}

void InspectorPanel::apply_dpi(framework::Dpi dpi, const framework::FontSet& fonts) {
    placeholder_.set_font(fonts.regular().handle());
    library_.apply_dpi(dpi, fonts);
    playlist_.apply_dpi(dpi, fonts);
    layout();
}

void InspectorPanel::show_first_run() {
    show_content(Content::FirstRun);
}

void InspectorPanel::show_library(const std::filesystem::path& folder, const TrackCounts& counts) {
    library_.set_contents(folder, counts);
    show_content(Content::Library);
}

void InspectorPanel::show_playlist(const PlaylistRow& playlist) {
    playlist_.set_contents(playlist);
    show_content(Content::Playlist);
}

void InspectorPanel::update(const CommandState& state) {
    library_.update(state);
    playlist_.update(state);
}

LRESULT InspectorPanel::handle_message(UINT message, WPARAM wparam, LPARAM lparam) {
    switch (message) {
    case WM_SIZE:       layout(); return 0;
    case WM_VSCROLL:    scrolled(scroll_.on_scroll(hwnd(), LOWORD(wparam))); return 0;
    case WM_MOUSEWHEEL: scrolled(scroll_.on_wheel(hwnd(), GET_WHEEL_DELTA_WPARAM(wparam))); return 0;
    default:            return Panel::handle_message(message, wparam, lparam);
    }
}

void InspectorPanel::show_content(Content content) {
    // Every card starts at its top
    content_ = content;
    scroll_.scroll_to(hwnd(), 0);
    layout();
}

framework::Rect InspectorPanel::padded_area() const {
    RECT client;
    platform::win32::check(GetClientRect(hwnd(), &client), "GetClientRect");
    framework::Rect area = framework::Rect::from_rect(client);
    framework::Dpi dpi = framework::Dpi::of(hwnd());
    area.take_left(dpi.scale(SidePaddingDips));
    area.take_right(dpi.scale(SidePaddingDips));
    area.take_top(dpi.scale(TopPaddingDips));
    area.take_bottom(dpi.scale(BottomPaddingDips));
    return area;
}

int InspectorPanel::card_height(int width) const {
    switch (content_) {
    case Content::FirstRun: return 0;
    case Content::Library:  return library_.height_for(width);
    case Content::Playlist: return playlist_.height_for(width);
    }
    std::unreachable();
}

void InspectorPanel::layout() {
    framework::Dpi dpi = framework::Dpi::of(hwnd());
    int padding = dpi.scale(TopPaddingDips) + dpi.scale(BottomPaddingDips);
    // Showing or hiding the scroll bar sends WM_SIZE, which lays out again before this goes on with the new width
    scroll_.update(hwnd(), content_ == Content::FirstRun ? 0 : padding + card_height(padded_area().width()));

    framework::Rect area = padded_area();
    framework::DeferredPositions positions;
    if (content_ == Content::FirstRun) {
        int height = std::min(placeholder_.height_for(area.width()), area.height());
        int top = area.top + (area.height() - height) / 2;
        positions.place(placeholder_.hwnd(), framework::Rect{area.left, top, area.right, top + height});
    } else {
        positions.place(placeholder_.hwnd(), std::nullopt);
    }

    int top = area.top - scroll_.position();
    framework::Rect card{area.left, top, area.right, top + card_height(area.width())};
    positions.place(library_.hwnd(), content_ == Content::Library ? std::optional(card) : std::nullopt);
    positions.place(playlist_.hwnd(), content_ == Content::Playlist ? std::optional(card) : std::nullopt);
    positions.apply();
}

void InspectorPanel::scrolled(bool moved) {
    if (moved) {
        layout();
    }
}

}
