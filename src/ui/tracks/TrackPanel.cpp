#include "ui/tracks/TrackPanel.h"

#include <algorithm>
#include <string_view>
#include <utility>

#include <windows.h>
#include <commctrl.h>

#include "platform/win32/Win32Error.h"
#include "ui/framework/BufferedPaint.h"
#include "ui/framework/DeferredPositions.h"
#include "ui/framework/Rect.h"
#include "ui/tracks/ResultLineText.h"

namespace westie::ui {

namespace {

constexpr int StatusTopDips = 4;
constexpr int StatusSideDips = 8;
constexpr int StatusBottomDips = 2;
constexpr int ResultTopDips = 3;
constexpr int ResultSideDips = 9;
constexpr int ResultBottomDips = 5;
constexpr int ResultLineDips = 16;
// A hairline stays one pixel at every scale
constexpr int HairlinePixels = 1;
constexpr COLORREF RuleColor = RGB(0xE5, 0xE5, 0xE5);

framework::Rect inset(framework::Rect rect, int side) {
    rect.take_left(side);
    rect.take_right(side);
    return rect;
}

}

TrackPanel::TrackPanel(HWND parent) :
        Panel(parent, L"Tracks"),
        filter_(hwnd(), [this] { update_clear(); }, [this] { clear_status(); }),
        status_(hwnd()),
        result_(hwnd(), L"", {.color = framework::TextColor::Gray}),
        table_(hwnd()) {
    show_result_line();
}

void TrackPanel::apply_dpi(framework::Dpi dpi, const framework::FontSet& fonts) {
    filter_.apply_dpi(dpi, fonts.regular());
    status_.apply_dpi(dpi, fonts.regular());
    result_.set_font(fonts.regular().handle());
    result_height_ = std::max(dpi.scale(ResultLineDips), fonts.regular().line_height());
    table_.apply_dpi(dpi, fonts.regular());
    layout();
}

void TrackPanel::show_scope(std::optional<std::wstring> playlist_name) {
    playlist_name_ = std::move(playlist_name);
    table_.show_scope(playlist_name_.has_value());
    show_result_line();
}

LRESULT TrackPanel::handle_message(UINT message, WPARAM wparam, LPARAM lparam) {
    switch (message) {
    case WM_SIZE:       layout(); return 0;
    // The paint fills the background, so the rule isn't erased first and doesn't blink
    case WM_ERASEBKGND: return TRUE;
    case WM_PAINT:      paint(); return 0;
    case WM_NOTIFY:     return on_notify(*reinterpret_cast<NMHDR*>(lparam));
    default:            return Panel::handle_message(message, wparam, lparam);
    }
}

void TrackPanel::layout() {
    RECT client;
    platform::win32::check(GetClientRect(hwnd(), &client), "GetClientRect");
    framework::Dpi dpi = framework::Dpi::of(hwnd());
    framework::Rect area = framework::Rect::from_rect(client);

    framework::Rect filter = area.take_top(filter_.height());
    area.take_top(dpi.scale(StatusTopDips));
    framework::Rect status = inset(area.take_top(status_.height()), dpi.scale(StatusSideDips));
    area.take_top(dpi.scale(StatusBottomDips) + dpi.scale(ResultTopDips));
    framework::Rect result = inset(area.take_top(result_height_), dpi.scale(ResultSideDips));
    area.take_top(dpi.scale(ResultBottomDips));
    framework::Rect rule = area.take_top(HairlinePixels);
    if (rule.top != rule_top_) {
        rule_top_ = rule.top;
        platform::win32::check(InvalidateRect(hwnd(), nullptr, FALSE), "InvalidateRect");
    }

    framework::DeferredPositions positions;
    positions.place(filter_.hwnd(), filter);
    positions.place(status_.hwnd(), status);
    positions.place(result_.hwnd(), result);
    positions.place(table_.hwnd(), area);
    positions.apply();
}

void TrackPanel::paint() const {
    framework::BufferedPaint paint(hwnd());
    RECT client;
    platform::win32::check(GetClientRect(hwnd(), &client), "GetClientRect");
    FillRect(paint.dc(), &client, GetSysColorBrush(COLOR_WINDOW));
    RECT rule{client.left, rule_top_, client.right, rule_top_ + HairlinePixels};
    SetDCBrushColor(paint.dc(), RuleColor);
    FillRect(paint.dc(), &rule, static_cast<HBRUSH>(GetStockObject(DC_BRUSH)));
}

LRESULT TrackPanel::on_notify(NMHDR& header) {
    if (header.hwndFrom == table_.hwnd()) {
        return table_.on_notify(header);
    }
    if (header.hwndFrom == status_.hwnd() && header.code == TCN_SELCHANGE) {
        status_.on_selection_change();
        update_clear();
    }
    return 0;
}

void TrackPanel::update_clear() {
    filter_.set_clear_enabled(!filter_.is_default() || status_.selected() != StatusTab::All);
}

void TrackPanel::clear_status() {
    status_.reset();
    update_clear();
}

void TrackPanel::show_result_line() {
    // Until there are tracks, both counts are 0
    std::optional<std::wstring_view> playlist_name = playlist_name_ ? std::optional<std::wstring_view>(*playlist_name_) : std::nullopt;
    result_.set_text(result_line_text(0, 0, playlist_name));
}

}
