#include "ui/tracks/FilterBar.h"

#include <algorithm>
#include <array>
#include <utility>

#include "ResourceIds.h"
#include "platform/win32/Win32Error.h"
#include "ui/framework/DeferredPositions.h"
#include "ui/framework/Rect.h"

namespace westie::ui {

namespace {

constexpr int PaddingDips = 8;
constexpr int BottomPaddingDips = 4;
constexpr int GapDips = 8;
constexpr int SearchWidthDips = 300;
constexpr int SourceWidthDips = 128;
// Where the edit's text would start without the icon, so the icon sits 5 DIPs inside the border
constexpr int SearchIconLeftDips = 4;
constexpr int SearchIconDips = 16;
constexpr int SearchIconGapDips = 5;

constexpr std::array Sources = {L"All", L"YouTube Music", L"Spotify"};

framework::Rect centered(const framework::Rect& slot, int height) {
    int top = slot.top + (slot.height() - height) / 2;
    return {slot.left, top, slot.right, top + height};
}

}

FilterBar::FilterBar(HWND parent, std::function<void()> on_change, std::function<void()> on_clear) :
        Panel(parent, L"Filter bar"),
        on_change_(std::move(on_change)),
        on_clear_(std::move(on_clear)),
        search_(hwnd(), {.cue_banner = L"Search title, artist, album", .leading_dips = SearchIconDips + SearchIconGapDips, .escape_clears = true}),
        search_icon_(search_.hwnd(), IDI_SEARCH, SearchIconDips),
        source_label_(hwnd(), L"Source"),
        source_(hwnd()),
        clear_(hwnd(), {.text = L"Clear", .compact = true}) {
    source_.add_items(Sources);
    source_.select(0);
    set_clear_enabled(false);
}

int FilterBar::height() const {
    framework::Dpi dpi = framework::Dpi::of(hwnd());
    return dpi.scale(PaddingDips) + row_height() + dpi.scale(BottomPaddingDips);
}

void FilterBar::apply_dpi(framework::Dpi dpi, const framework::MessageFont& font) {
    search_.apply_dpi(dpi, font);
    search_icon_.apply_dpi(dpi);
    source_label_.set_font(font.handle());
    // Just the line, so centering it puts the text level with the search field's, which is centered the same way
    label_height_ = font.line_height();
    source_.apply_dpi(dpi, font);
    clear_.apply_dpi(dpi, font);
    layout();
}

bool FilterBar::is_default() const {
    return search_.text().empty() && source_.selected() == 0;
}

std::wstring FilterBar::search() const {
    return search_.text();
}

std::optional<core::SourceType> FilterBar::source() const {
    int selected = source_.selected();
    // The list starts with All, then the sources in their order
    if (selected <= 0 || std::cmp_greater(selected, core::AllSourceTypes.size())) {
        return std::nullopt;
    }
    return core::AllSourceTypes[selected - 1];
}

void FilterBar::clear() {
    search_.set_text(L"");
    source_.select(0);
}

void FilterBar::set_clear_enabled(bool enabled) {
    clear_.set_enabled(enabled);
}

void FilterBar::focus_search() {
    SetFocus(search_.hwnd());
    search_.select_all();
}

LRESULT FilterBar::handle_message(UINT message, WPARAM wparam, LPARAM lparam) {
    switch (message) {
    case WM_SIZE:    layout(); return 0;
    case WM_COMMAND: on_command(wparam, lparam); return 0;
    default:         return Panel::handle_message(message, wparam, lparam);
    }
}

int FilterBar::row_height() const {
    return std::max({search_.height(), source_.height(), clear_.size().height});
}

void FilterBar::layout() {
    RECT client;
    platform::win32::check(GetClientRect(hwnd(), &client), "GetClientRect");
    framework::Dpi dpi = framework::Dpi::of(hwnd());
    framework::Rect row = framework::Rect::from_rect(client);
    row.take_top(dpi.scale(PaddingDips));
    row = row.take_top(row_height());
    row.take_left(dpi.scale(PaddingDips));
    row.take_right(dpi.scale(PaddingDips));

    // The search field gives way when the panel is too narrow for everything
    int gap = dpi.scale(GapDips);
    int label_width = source_label_.text_width();
    int rest = gap + label_width + gap + dpi.scale(SourceWidthDips) + gap + clear_.size().width;
    framework::Rect search = row.take_left(std::clamp(row.width() - rest, 0, dpi.scale(SearchWidthDips)));
    row.take_left(gap);
    framework::Rect label = row.take_left(label_width);
    row.take_left(gap);
    framework::Rect source = row.take_left(dpi.scale(SourceWidthDips));
    row.take_left(gap);
    framework::Rect clear = row.take_left(clear_.size().width);

    framework::DeferredPositions positions;
    positions.place(search_.hwnd(), centered(search, search_.height()));
    positions.place(source_label_.hwnd(), centered(label, label_height_));
    positions.place(source_.hwnd(), centered(source, source_.height()));
    positions.place(clear_.hwnd(), centered(clear, clear_.size().height));
    positions.apply();
    place_search_icon();
}

void FilterBar::place_search_icon() {
    // Centered in the field, not in its client area, which starts lower to center the text
    RECT window;
    platform::win32::check(GetWindowRect(search_.hwnd(), &window), "GetWindowRect");
    POINT client{};
    platform::win32::check(ClientToScreen(search_.hwnd(), &client), "ClientToScreen");
    int size = search_icon_.size();
    int left = framework::Dpi::of(hwnd()).scale(SearchIconLeftDips);
    int top = std::max(0, static_cast<int>(window.bottom - window.top - size) / 2 - static_cast<int>(client.y - window.top));
    platform::win32::check(SetWindowPos(search_icon_.hwnd(), nullptr, left, top, size, size, SWP_NOZORDER | SWP_NOACTIVATE), "SetWindowPos");
}

void FilterBar::on_command(WPARAM wparam, LPARAM lparam) {
    auto control = reinterpret_cast<HWND>(lparam);
    if ((control == search_.hwnd() && HIWORD(wparam) == EN_CHANGE) || (control == source_.hwnd() && HIWORD(wparam) == CBN_SELCHANGE)) {
        on_change_();
    } else if (control == clear_.hwnd() && HIWORD(wparam) == BN_CLICKED) {
        // Clear grays out once everything is back at its default, and a disabled button can't keep the focus
        if (GetFocus() == clear_.hwnd()) {
            SetFocus(search_.hwnd());
        }
        clear();
        on_clear_();
    }
}

}
