#include "ui/inspector/InspectorPanel.h"

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

InspectorPanel::InspectorPanel(HWND parent, const InspectorEvents& events) :
        Panel(parent, L"Inspector", WS_VSCROLL),
        library_(hwnd(), events.on_command),
        playlist_(hwnd(), events.on_command),
        track_(hwnd(), events),
        selection_(hwnd()) {}

void InspectorPanel::apply_dpi(framework::Dpi dpi, const framework::FontSet& fonts) {
    library_.apply_dpi(dpi, fonts);
    playlist_.apply_dpi(dpi, fonts);
    track_.apply_dpi(dpi, fonts);
    selection_.apply_dpi(dpi, fonts);
    layout();
}

void InspectorPanel::show_first_run() {
    show_content(Content::FirstRun, {});
}

void InspectorPanel::show_library(const std::filesystem::path& folder, const TrackCounts& counts) {
    library_.set_contents(folder, counts);
    show_content(Content::Library, {});
}

void InspectorPanel::show_playlist(const PlaylistRow& playlist) {
    playlist_.set_contents(playlist);
    show_content(Content::Playlist, playlist.key);
}

void InspectorPanel::show_track(const TrackDetails& track) {
    track_.set_contents(track);
    show_content(Content::Track, track.row.key);
}

void InspectorPanel::show_selection(const StatusCounts& counts) {
    selection_.set_contents(counts);
    show_content(Content::Selection, {});
}

void InspectorPanel::update(const CommandState& state) {
    library_.update(state);
    playlist_.update(state);
    track_.update(state);
}

LRESULT InspectorPanel::handle_message(UINT message, WPARAM wparam, LPARAM lparam) {
    switch (message) {
    case WM_SIZE:       layout(); return 0;
    case WM_VSCROLL:    scrolled(scroll_.on_scroll(hwnd(), LOWORD(wparam))); return 0;
    case WM_MOUSEWHEEL: scrolled(scroll_.on_wheel(hwnd(), GET_WHEEL_DELTA_WPARAM(wparam))); return 0;
    default:            return Panel::handle_message(message, wparam, lparam);
    }
}

void InspectorPanel::show_content(Content content, const std::string& subject) {
    if (content != content_ || subject != subject_) {
        content_ = content;
        subject_ = subject;
        scroll_.scroll_to(hwnd(), 0);
    }
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
    case Content::FirstRun:  return 0;
    case Content::Library:   return library_.height_for(width);
    case Content::Playlist:  return playlist_.height_for(width);
    case Content::Track:     return track_.height_for(width);
    case Content::Selection: return selection_.height_for(width);
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

    int top = area.top - scroll_.position();
    framework::Rect card{area.left, top, area.right, top + card_height(area.width())};
    positions.place(library_.hwnd(), content_ == Content::Library ? std::optional(card) : std::nullopt);
    positions.place(playlist_.hwnd(), content_ == Content::Playlist ? std::optional(card) : std::nullopt);
    positions.place(track_.hwnd(), content_ == Content::Track ? std::optional(card) : std::nullopt);
    positions.place(selection_.hwnd(), content_ == Content::Selection ? std::optional(card) : std::nullopt);
    positions.apply();
}

void InspectorPanel::scrolled(bool moved) {
    if (moved) {
        layout();
    }
}

}
