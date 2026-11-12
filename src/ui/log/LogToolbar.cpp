#include "ui/log/LogToolbar.h"

#include <algorithm>
#include <array>
#include <string>
#include <utility>
#include <vector>

#include "platform/win32/Win32Error.h"
#include "ui/framework/BufferedPaint.h"
#include "ui/framework/DeferredPositions.h"
#include "ui/framework/Rect.h"

namespace westie::ui {

namespace {

// 28 with the rule under it
constexpr int StripDips = 27;
constexpr int PaddingDips = 2;
constexpr int LeftPaddingDips = 8;
constexpr int RightPaddingDips = 4;
constexpr int TitleGapDips = 12;
constexpr int LabelGapDips = 6;
constexpr int GroupGapDips = 12;
constexpr int CloseGapDips = 6;
constexpr int LevelWidthDips = 128;
constexpr int ScopeHeightDips = 20;
// A hairline stays one pixel at every scale
constexpr int HairlinePixels = 1;
constexpr COLORREF RuleColor = RGB(0xE5, 0xE5, 0xE5);

// In the order of the levels, each showing itself and the more severe ones
constexpr std::array LevelTexts = {L"Debug and up", L"Info and up", L"Warnings and up", L"Errors only"};
// In the order of the scopes
constexpr std::array ScopeTexts = {L"All", L"Selected playlist", L"Selected track"};

framework::Rect centered(const framework::Rect& slot, int height) {
    int top = slot.top + (slot.height() - height) / 2;
    return {slot.left, top, slot.right, top + height};
}

}

LogToolbar::LogToolbar(HWND parent, Callbacks callbacks) :
        Panel(parent, L"Log toolbar"),
        callbacks_(std::move(callbacks)),
        title_(hwnd(), L"Log"),
        level_label_(hwnd(), L"Level"),
        level_(hwnd()),
        scope_(hwnd(), ScopeHeightDips),
        open_folder_(hwnd(), {.text = L"Open log folder", .compact = true}),
        close_(hwnd(), GetSysColor(COLOR_WINDOW), [this] { callbacks_.on_close(); }) {
    level_.add_items(LevelTexts);
    set_view({});
}

int LogToolbar::height() const {
    framework::Dpi dpi = framework::Dpi::of(hwnd());
    return std::max(dpi.scale(StripDips), row_height() + 2 * dpi.scale(PaddingDips)) + HairlinePixels;
}

void LogToolbar::apply_dpi(framework::Dpi dpi, const framework::FontSet& fonts) {
    title_.set_font(fonts.semibold().handle());
    title_line_height_ = fonts.semibold().line_height();
    level_label_.set_font(fonts.regular().handle());
    line_height_ = fonts.regular().line_height();
    level_.apply_dpi(dpi, fonts.regular());
    scope_.apply_dpi(dpi, fonts.regular());
    open_folder_.apply_dpi(dpi, fonts.regular());
    close_.apply_dpi(dpi);
    layout();
}

LogView LogToolbar::view() const {
    int level = std::clamp(level_.selected(), 0, static_cast<int>(LevelTexts.size()) - 1);
    int scope = std::clamp(scope_.selected(), 0, static_cast<int>(ScopeTexts.size()) - 1);
    return {.least = static_cast<core::LogLevel>(level), .scope = static_cast<LogScope>(scope)};
}

void LogToolbar::set_view(LogView view) {
    level_.select(std::to_underlying(view.least));
    std::vector<std::wstring> scopes(ScopeTexts.begin(), ScopeTexts.end());
    scope_.set_tabs(scopes, std::to_underlying(view.scope));
}

LRESULT LogToolbar::handle_message(UINT message, WPARAM wparam, LPARAM lparam) {
    switch (message) {
    case WM_SIZE:       layout(); return 0;
    // The paint fills the background, so the rule isn't erased first and doesn't blink
    case WM_ERASEBKGND: return TRUE;
    case WM_PAINT:      paint(); return 0;
    case WM_COMMAND:    on_command(wparam, lparam); return 0;
    case WM_NOTIFY:     return on_notify(*reinterpret_cast<const NMHDR*>(lparam));
    default:            return Panel::handle_message(message, wparam, lparam);
    }
}

int LogToolbar::row_height() const {
    // The close box may reach the strip's edges, as in the infobar
    return std::max({level_.height(), scope_.height(), open_folder_.size().height});
}

void LogToolbar::layout() {
    RECT client;
    platform::win32::check(GetClientRect(hwnd(), &client), "GetClientRect");
    framework::Dpi dpi = framework::Dpi::of(hwnd());
    framework::Rect row = framework::Rect::from_rect(client);
    row.take_bottom(HairlinePixels);
    row.take_left(dpi.scale(LeftPaddingDips));
    row.take_right(dpi.scale(RightPaddingDips));

    framework::Rect title = row.take_left(title_.text_width());
    row.take_left(dpi.scale(TitleGapDips));
    framework::Rect label = row.take_left(level_label_.text_width());
    row.take_left(dpi.scale(LabelGapDips));
    framework::Rect level = row.take_left(dpi.scale(LevelWidthDips));
    row.take_left(dpi.scale(GroupGapDips));
    framework::Rect close = row.take_right(close_.size());
    row.take_right(dpi.scale(CloseGapDips));
    framework::Rect open_folder = row.take_right(open_folder_.size().width);
    row.take_right(dpi.scale(GroupGapDips));

    framework::DeferredPositions positions;
    positions.place(title_.hwnd(), centered(title, title_line_height_));
    positions.place(level_label_.hwnd(), centered(label, line_height_));
    positions.place(level_.hwnd(), centered(level, level_.height()));
    // The rest of the row, between the list and the button; the buttons sit at its left
    positions.place(scope_.hwnd(), centered(row, scope_.height()));
    positions.place(open_folder_.hwnd(), centered(open_folder, open_folder_.size().height));
    positions.place(close_.hwnd(), centered(close, close_.size()));
    positions.apply();
    platform::win32::check(InvalidateRect(hwnd(), nullptr, FALSE), "InvalidateRect");
}

void LogToolbar::paint() const {
    framework::BufferedPaint paint(hwnd());
    RECT client;
    platform::win32::check(GetClientRect(hwnd(), &client), "GetClientRect");
    FillRect(paint.dc(), &client, GetSysColorBrush(COLOR_WINDOW));
    RECT rule{client.left, client.bottom - HairlinePixels, client.right, client.bottom};
    SetDCBrushColor(paint.dc(), RuleColor);
    FillRect(paint.dc(), &rule, static_cast<HBRUSH>(GetStockObject(DC_BRUSH)));
}

void LogToolbar::on_command(WPARAM wparam, LPARAM lparam) {
    auto control = reinterpret_cast<HWND>(lparam);
    if (control == level_.hwnd() && HIWORD(wparam) == CBN_SELCHANGE) {
        callbacks_.on_view_change(view());
    } else if (control == open_folder_.hwnd() && HIWORD(wparam) == BN_CLICKED) {
        callbacks_.on_open_folder();
    }
}

LRESULT LogToolbar::on_notify(const NMHDR& header) {
    if (header.hwndFrom == scope_.hwnd() && header.code == TCN_SELCHANGE) {
        callbacks_.on_view_change(view());
    }
    return 0;
}

}
