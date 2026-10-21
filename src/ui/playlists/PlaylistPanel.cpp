#include "ui/playlists/PlaylistPanel.h"

#include <algorithm>
#include <utility>

#include "ResourceIds.h"
#include "platform/win32/Win32Error.h"
#include "ui/framework/BufferedPaint.h"
#include "ui/framework/DeferredPositions.h"
#include "ui/framework/Size.h"

namespace westie::ui {

namespace {

constexpr int RowInsetDips = 3;
constexpr int RowTopDips = 4;
constexpr int RuleGapDips = 5;
constexpr int RuleInsetDips = 10;
constexpr int EmptyTextTopDips = 6;
constexpr int EmptyTextRightDips = 12;
constexpr int EmptyTextBottomDips = 2;
constexpr int EmptyTextLineDips = 16;
constexpr int ButtonTopDips = 8;
// A hairline stays one pixel at every scale
constexpr int HairlinePixels = 1;
constexpr COLORREF RuleColor = RGB(0xE5, 0xE5, 0xE5);

}

PlaylistPanel::PlaylistPanel(HWND parent, std::function<void(CommandId)> on_command) :
        Panel(parent, L"Playlists"),
        on_command_(std::move(on_command)),
        all_tracks_(hwnd()),
        empty_text_(hwnd(), L"No playlists yet.", {.color = framework::TextColor::Gray}),
        add_button_(hwnd(), {.text = L"Add playlist", .command = std::to_underlying(CommandId::AddPlaylist), .icon = IDI_PLUS}) {}

void PlaylistPanel::apply_dpi(framework::Dpi dpi, const framework::FontSet& fonts) {
    all_tracks_.apply_dpi(dpi, fonts);
    empty_text_.set_font(fonts.regular().handle());
    empty_text_height_ = std::max(dpi.scale(EmptyTextLineDips), fonts.regular().line_height());
    add_button_.apply_dpi(dpi, fonts.regular());
    layout();
    // The rule moves with the row's height
    platform::win32::check(InvalidateRect(hwnd(), nullptr, TRUE), "InvalidateRect");
}

void PlaylistPanel::set_track_counts(const TrackCounts& counts) {
    all_tracks_.set_counts(counts);
}

LRESULT PlaylistPanel::handle_message(UINT message, WPARAM wparam, LPARAM lparam) {
    switch (message) {
    case WM_SIZE:       layout(); return 0;
    // The paint fills the background, so the rule isn't erased first and doesn't blink
    case WM_ERASEBKGND: return TRUE;
    case WM_PAINT:      paint(); return 0;
    case WM_COMMAND:    on_command(wparam); return 0;
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

    // The empty list's text and the button line up with the All tracks name
    int text_left = row.left + all_tracks_.text_left();
    int empty_top = rule_.bottom + dpi.scale(RuleGapDips) + dpi.scale(EmptyTextTopDips);
    framework::Rect empty{text_left, empty_top, area.right - dpi.scale(EmptyTextRightDips), empty_top + empty_text_height_};

    framework::Size button = add_button_.size();
    int button_top = empty.bottom + dpi.scale(EmptyTextBottomDips) + dpi.scale(ButtonTopDips);
    framework::Rect add{text_left, button_top, text_left + button.width, button_top + button.height};

    framework::DeferredPositions positions;
    positions.place(all_tracks_.hwnd(), row);
    positions.place(empty_text_.hwnd(), empty);
    positions.place(add_button_.hwnd(), add);
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

void PlaylistPanel::on_command(WPARAM wparam) const {
    if (HIWORD(wparam) == BN_CLICKED) {
        on_command_(static_cast<CommandId>(LOWORD(wparam)));
    }
}

}
