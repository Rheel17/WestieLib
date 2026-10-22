#include "ui/playlists/PlaylistPanel.h"

#include <algorithm>
#include <utility>

#include "platform/win32/Win32Error.h"
#include "ui/framework/BufferedPaint.h"
#include "ui/framework/DeferredPositions.h"

namespace westie::ui {

namespace {

constexpr int RowInsetDips = 3;
constexpr int RowTopDips = 4;
constexpr int RuleGapDips = 5;
constexpr int RuleInsetDips = 10;
// A hairline stays one pixel at every scale
constexpr int HairlinePixels = 1;
constexpr COLORREF RuleColor = RGB(0xE5, 0xE5, 0xE5);

}

PlaylistPanel::PlaylistPanel(HWND parent, std::function<void(CommandId)> on_command) :
        Panel(parent, L"Playlists"), all_tracks_(hwnd()), list_(hwnd(), std::move(on_command)) {}

void PlaylistPanel::apply_dpi(framework::Dpi dpi, const framework::FontSet& fonts) {
    all_tracks_.apply_dpi(dpi, fonts);
    list_.apply_dpi(dpi, fonts);
    layout();
    // The rule moves with the row's height
    platform::win32::check(InvalidateRect(hwnd(), nullptr, TRUE), "InvalidateRect");
}

void PlaylistPanel::set_track_counts(const TrackCounts& counts) {
    all_tracks_.set_counts(counts);
}

void PlaylistPanel::set_playlists(std::vector<PlaylistRow> rows) {
    list_.set_rows(std::move(rows));
}

LRESULT PlaylistPanel::handle_message(UINT message, WPARAM wparam, LPARAM lparam) {
    switch (message) {
    case WM_SIZE:       layout(); return 0;
    // The paint fills the background, so the rule isn't erased first and doesn't blink
    case WM_ERASEBKGND: return TRUE;
    case WM_PAINT:      paint(); return 0;
    default:            return Panel::handle_message(message, wparam, lparam);
    }
}

void PlaylistPanel::layout() {
    RECT client;
    platform::win32::check(GetClientRect(hwnd(), &client), "GetClientRect");
    framework::Rect area = framework::Rect::from_rect(client);
    framework::Dpi dpi = framework::Dpi::of(hwnd());

    int row_top = dpi.scale(RowTopDips);
    framework::Rect row{dpi.scale(RowInsetDips), row_top, area.right - dpi.scale(RowInsetDips), row_top + all_tracks_.height()};

    int rule_top = row.bottom + dpi.scale(RuleGapDips);
    rule_ = {dpi.scale(RuleInsetDips), rule_top, area.right - dpi.scale(RuleInsetDips), rule_top + HairlinePixels};

    // Only the playlists scroll; All tracks stays where it is
    int list_top = rule_.bottom + dpi.scale(RuleGapDips);
    framework::DeferredPositions positions;
    positions.place(all_tracks_.hwnd(), row);
    positions.place(list_.hwnd(), framework::Rect{0, list_top, area.right, std::max(list_top, area.bottom)});
    positions.apply();
}

void PlaylistPanel::paint() const {
    framework::BufferedPaint paint(hwnd());
    RECT client;
    platform::win32::check(GetClientRect(hwnd(), &client), "GetClientRect");
    FillRect(paint.dc(), &client, GetSysColorBrush(COLOR_WINDOW));
    RECT rule = rule_.to_rect();
    SetDCBrushColor(paint.dc(), RuleColor);
    FillRect(paint.dc(), &rule, static_cast<HBRUSH>(GetStockObject(DC_BRUSH)));
}

}
