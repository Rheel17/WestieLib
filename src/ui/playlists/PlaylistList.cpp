#include "ui/playlists/PlaylistList.h"

#include <algorithm>
#include <chrono>
#include <utility>

#include <windows.h>
#include <windowsx.h>

#include "ResourceIds.h"
#include "platform/win32/Win32Error.h"
#include "ui/RowLook.h"
#include "ui/framework/BufferedPaint.h"
#include "ui/framework/DeferredPositions.h"
#include "ui/framework/Size.h"
#include "ui/framework/WindowClass.h"

namespace westie::ui {

namespace {

constexpr int RowInsetDips = 3;
constexpr int RowGapDips = 1;
constexpr int EmptyTextTopDips = 6;
constexpr int EmptyTextRightDips = 12;
constexpr int EmptyTextBottomDips = 2;
constexpr int EmptyTextLineDips = 16;
constexpr int ButtonTopDips = 8;
constexpr int BottomPaddingDips = 11;
constexpr UINT_PTR SpinTimer = 1;
constexpr UINT SpinIntervalMs = 100;
constexpr std::chrono::milliseconds SpinTurn{1200};

bool spins(const PlaylistRow& row) {
    return row.run == PlaylistRun::Fetching && row.waiting_for.empty();
}

}

PlaylistList::PlaylistList(HWND parent, PlaylistEvents events) :
        Window(window_class(), {.title = L"Playlist list", .style = WS_CHILD | WS_VISIBLE | WS_VSCROLL | WS_CLIPCHILDREN | WS_CLIPSIBLINGS, .parent = parent}),
        events_(std::move(events)),
        empty_text_(hwnd(), L"No playlists yet.", {.color = framework::TextColor::Gray}),
        add_button_(hwnd(), {.text = L"Add playlist", .command = std::to_underlying(CommandId::AddPlaylist), .icon = IDI_PLUS}),
        tips_(hwnd()),
        spin_(SpinTimer, SpinIntervalMs) {}

void PlaylistList::apply_dpi(framework::Dpi dpi, const framework::FontSet& fonts) {
    painter_.emplace(dpi, fonts);
    empty_text_.set_font(fonts.regular().handle());
    empty_text_height_ = std::max(dpi.scale(EmptyTextLineDips), fonts.regular().line_height());
    add_button_.apply_dpi(dpi, fonts.regular());
    layout();
}

void PlaylistList::set_rows(std::vector<PlaylistRow> rows) {
    rows_ = std::move(rows);
    spin_.run(hwnd(), std::ranges::any_of(rows_, spins));
    layout();
    follow_cursor();
}

void PlaylistList::set_selection(const std::optional<std::string>& key) {
    if (key == selected_) {
        return;
    }
    selected_ = key;
    reveal_selection();
    platform::win32::check(InvalidateRect(hwnd(), nullptr, FALSE), "InvalidateRect");
}

void PlaylistList::set_sync_enabled(bool enabled) {
    if (enabled == sync_enabled_) {
        return;
    }
    sync_enabled_ = enabled;
    place_tips();
    follow_cursor();
}

LRESULT PlaylistList::handle_message(UINT message, WPARAM wparam, LPARAM lparam) {
    switch (message) {
    case WM_SIZE: layout(); return 0;
    // The paint fills the background, so the rows aren't erased first and don't blink
    case WM_ERASEBKGND:     return TRUE;
    case WM_PAINT:          paint(); return 0;
    case WM_MOUSEMOVE:      track_mouse(point_of(lparam)); return 0;
    case WM_MOUSELEAVE:     on_mouse_leave(); return 0;
    case WM_LBUTTONDOWN:    on_left_button_down(point_of(lparam)); return 0;
    case WM_LBUTTONUP:      on_left_button_up(point_of(lparam)); return 0;
    case WM_CAPTURECHANGED: on_capture_changed(); return 0;
    case WM_TIMER:          on_timer(wparam); return 0;
    case WM_KEYDOWN:        return on_key_down(wparam) ? 0 : Window::handle_message(message, wparam, lparam);
    case WM_CONTEXTMENU:    on_context_menu(point_of(lparam)); return 0;
    case WM_VSCROLL:        scrolled(scroll_.on_scroll(hwnd(), LOWORD(wparam))); return 0;
    case WM_MOUSEWHEEL:     scrolled(scroll_.on_wheel(hwnd(), GET_WHEEL_DELTA_WPARAM(wparam))); return 0;
    case WM_COMMAND:        on_command(wparam); return 0;
    default:                return Window::handle_message(message, wparam, lparam);
    }
}

const framework::WindowClass& PlaylistList::window_class() {
    static framework::WindowClass window_class({.name = L"WestieLib.PlaylistList", .style = CS_HREDRAW | CS_VREDRAW});
    return window_class;
}

framework::Rect PlaylistList::client() const {
    RECT client;
    platform::win32::check(GetClientRect(hwnd(), &client), "GetClientRect");
    return framework::Rect::from_rect(client);
}

framework::Rect PlaylistList::row_bounds(std::size_t index) const {
    // A row with counts that need attention is a line taller than one without
    framework::Dpi dpi = framework::Dpi::of(hwnd());
    int top = dpi.scale(RowGapDips) - scroll_.position();
    for (std::size_t above = 0; above < index; ++above) {
        top += painter_->height(rows_[above]) + dpi.scale(RowGapDips);
    }
    return {dpi.scale(RowInsetDips), top, client().right - dpi.scale(RowInsetDips), top + painter_->height(rows_[index])};
}

int PlaylistList::rows_bottom() const {
    if (rows_.empty()) {
        return 0;
    }
    return row_bounds(rows_.size() - 1).bottom + framework::Dpi::of(hwnd()).scale(RowGapDips) + scroll_.position();
}

int PlaylistList::content_height() const {
    framework::Dpi dpi = framework::Dpi::of(hwnd());
    int above_button = rows_.empty() ? dpi.scale(EmptyTextTopDips) + empty_text_height_ + dpi.scale(EmptyTextBottomDips) : rows_bottom();
    return above_button + dpi.scale(ButtonTopDips) + add_button_.size().height + dpi.scale(BottomPaddingDips);
}

std::optional<std::size_t> PlaylistList::selected_index() const {
    if (!selected_) {
        return std::nullopt;
    }
    auto row = std::ranges::find(rows_, *selected_, &PlaylistRow::key);
    if (row == rows_.end()) {
        return std::nullopt;
    }
    return static_cast<std::size_t>(row - rows_.begin());
}

POINT PlaylistList::point_of(LPARAM lparam) {
    return {GET_X_LPARAM(lparam), GET_Y_LPARAM(lparam)};
}

std::optional<std::size_t> PlaylistList::row_at(POINT point) const {
    for (std::size_t index = 0; index < rows_.size(); ++index) {
        RECT bounds = row_bounds(index).to_rect();
        if (PtInRect(&bounds, point)) {
            return index;
        }
    }
    return std::nullopt;
}

std::optional<std::size_t> PlaylistList::sync_at(POINT point) const {
    std::optional<std::size_t> row = row_at(point);
    if (!row || !sync_enabled_) {
        return std::nullopt;
    }
    RECT button = painter_->sync_button(row_bounds(*row), rows_[*row], std::chrono::system_clock::now()).to_rect();
    return PtInRect(&button, point) ? row : std::nullopt;
}

void PlaylistList::layout() {
    if (!painter_) {
        return;
    }
    scroll_.update(hwnd(), content_height());

    framework::Dpi dpi = framework::Dpi::of(hwnd());
    framework::Rect area = client();
    // The empty list's text and the button line up with the playlist names, and so with All tracks above
    int text_left = dpi.scale(RowInsetDips) + painter_->text_left();
    int empty_top = dpi.scale(EmptyTextTopDips) - scroll_.position();
    framework::Rect empty{text_left, empty_top, area.right - dpi.scale(EmptyTextRightDips), empty_top + empty_text_height_};

    int button_top = (rows_.empty() ? empty.bottom + dpi.scale(EmptyTextBottomDips) : rows_bottom() - scroll_.position()) + dpi.scale(ButtonTopDips);
    framework::Size button = add_button_.size();

    platform::win32::check(InvalidateRect(hwnd(), nullptr, FALSE), "InvalidateRect");
    framework::DeferredPositions positions;
    positions.place(empty_text_.hwnd(), rows_.empty() ? std::optional(empty) : std::nullopt);
    positions.place(add_button_.hwnd(), framework::Rect{text_left, button_top, text_left + button.width, button_top + button.height});
    positions.apply();
    place_tips();
}

void PlaylistList::place_tips() {
    if (!painter_) {
        return;
    }
    std::chrono::system_clock::time_point now = std::chrono::system_clock::now();
    for (std::size_t index = 0; index < rows_.size(); ++index) {
        // Only a control that can be clicked explains itself
        if (sync_enabled_) {
            tips_.set_tool(index, painter_->sync_button(row_bounds(index), rows_[index], now), L"Sync " + rows_[index].name);
        } else {
            tips_.remove_tool(index);
        }
    }
    for (std::size_t index = rows_.size(); index < tip_count_; ++index) {
        tips_.remove_tool(index);
    }
    tip_count_ = rows_.size();
}

void PlaylistList::reveal_selection() {
    std::optional<std::size_t> index = selected_index();
    if (!index || !painter_) {
        return;
    }
    // The gaps around the row come along, so its frame doesn't touch the edge
    int gap = framework::Dpi::of(hwnd()).scale(RowGapDips);
    framework::Rect bounds = row_bounds(*index);
    scrolled(scroll_.reveal(hwnd(), bounds.top - gap + scroll_.position(), bounds.bottom + gap + scroll_.position()));
}

void PlaylistList::scrolled(bool moved) {
    if (moved) {
        layout();
        follow_cursor();
    }
}

void PlaylistList::follow_cursor() {
    // The rows move under a mouse that stands still, which sends no WM_MOUSEMOVE
    POINT point;
    if (!GetCursorPos(&point) || !painter_) {
        return;
    }
    bool over = WindowFromPoint(point) == hwnd();
    platform::win32::check(ScreenToClient(hwnd(), &point), "ScreenToClient");
    track_mouse(over ? std::optional(point) : std::nullopt);
}

void PlaylistList::track_mouse(std::optional<POINT> point) {
    bool changed = mouse_.move(hwnd(), point ? row_at(*point) : std::nullopt);
    std::optional<std::size_t> sync = point ? sync_at(*point) : std::nullopt;
    // While the control is held, only it reacts
    if (sync_pressed_ && sync != sync_pressed_) {
        sync.reset();
    }
    if (sync != sync_hot_) {
        sync_hot_ = sync;
        changed = true;
    }
    repaint_if(changed);
}

void PlaylistList::repaint_if(bool changed) const {
    if (changed) {
        platform::win32::check(InvalidateRect(hwnd(), nullptr, FALSE), "InvalidateRect");
    }
}

void PlaylistList::paint() const {
    framework::BufferedPaint paint(hwnd());
    framework::Rect area = client();
    RECT background = area.to_rect();
    FillRect(paint.dc(), &background, GetSysColorBrush(COLOR_WINDOW));
    if (!painter_) {
        return;
    }
    std::optional<std::size_t> selected = selected_index();
    auto spin_step = static_cast<int>(spin_.progress(SpinTurn) * PlaylistRowPainter::SpinSteps);
    std::chrono::system_clock::time_point now = std::chrono::system_clock::now();
    for (std::size_t index = 0; index < rows_.size(); ++index) {
        framework::Rect bounds = row_bounds(index);
        if (bounds.bottom > area.top && bounds.top < area.bottom) {
            RowPaint look{
                .look = row_look(index == selected, index == mouse_.hot(), index == mouse_.pressed()),
                .sync_hot = index == sync_hot_,
                .spin_step = spin_step,
                .now = now,
            };
            painter_->paint(paint.dc(), bounds, rows_[index], look);
        }
    }
}

void PlaylistList::on_mouse_leave() {
    bool changed = mouse_.leave();
    // The capture keeps the moves coming while the control is held
    if (!sync_pressed_ && sync_hot_) {
        sync_hot_.reset();
        changed = true;
    }
    repaint_if(changed);
}

void PlaylistList::on_left_button_down(POINT point) {
    SetFocus(hwnd());
    // The sync control syncs its playlist without selecting it
    if (std::optional<std::size_t> sync = sync_at(point)) {
        sync_pressed_ = sync;
        sync_hot_ = sync;
        SetCapture(hwnd());
        repaint_if(true);
        return;
    }
    repaint_if(mouse_.press(hwnd(), row_at(point)));
}

void PlaylistList::on_left_button_up(POINT point) {
    if (std::optional<std::size_t> pressed = sync_pressed_) {
        // Cleared first, so the WM_CAPTURECHANGED that ReleaseCapture sends finds nothing to cancel
        sync_pressed_.reset();
        platform::win32::check(ReleaseCapture(), "ReleaseCapture");
        track_mouse(point);
        if (sync_at(point) == pressed) {
            events_.on_sync(rows_[*pressed].key);
        }
        return;
    }
    std::optional<std::size_t> clicked = mouse_.release(hwnd(), row_at(point));
    repaint_if(true);
    if (clicked) {
        events_.on_select(rows_[*clicked].key);
    }
}

void PlaylistList::on_capture_changed() {
    bool changed = mouse_.cancel();
    if (sync_pressed_) {
        sync_pressed_.reset();
        sync_hot_.reset();
        changed = true;
    }
    repaint_if(changed);
}

void PlaylistList::on_timer(WPARAM timer) const {
    if (!spin_.is_tick(timer)) {
        return;
    }
    std::chrono::system_clock::time_point now = std::chrono::system_clock::now();
    for (std::size_t index = 0; index < rows_.size(); ++index) {
        if (spins(rows_[index])) {
            RECT button = painter_->sync_button(row_bounds(index), rows_[index], now).to_rect();
            platform::win32::check(InvalidateRect(hwnd(), &button, FALSE), "InvalidateRect");
        }
    }
}

bool PlaylistList::on_key_down(WPARAM key) {
    // All tracks comes before the first playlist
    int last = static_cast<int>(rows_.size());
    std::optional<std::size_t> selected = selected_index();
    int current = selected ? static_cast<int>(*selected) + 1 : 0;
    int page = std::max(1, client().height() / (painter_->height(PlaylistRow{}) + framework::Dpi::of(hwnd()).scale(RowGapDips)));

    int target = current;
    switch (key) {
    case VK_UP:    target = current - 1; break;
    case VK_DOWN:  target = current + 1; break;
    case VK_PRIOR: target = current - page; break;
    case VK_NEXT:  target = current + page; break;
    case VK_HOME:  target = 0; break;
    case VK_END:   target = last; break;
    default:       return false;
    }
    target = std::clamp(target, 0, last);
    if (target != current) {
        events_.on_select(target == 0 ? std::nullopt : std::optional(rows_[static_cast<std::size_t>(target - 1)].key));
    }
    return true;
}

void PlaylistList::on_context_menu(POINT screen) {
    // From the keyboard the point is (-1, -1), and the menu goes to the selected row
    if (screen.x == -1 && screen.y == -1) {
        std::optional<std::size_t> index = selected_index();
        if (!index) {
            return;
        }
        reveal_selection();
        framework::Rect bounds = row_bounds(*index);
        POINT corner{bounds.left, bounds.bottom};
        platform::win32::check(ClientToScreen(hwnd(), &corner), "ClientToScreen");
        events_.on_context_menu(corner);
        return;
    }

    POINT point = screen;
    platform::win32::check(ScreenToClient(hwnd(), &point), "ScreenToClient");
    std::optional<std::size_t> index = row_at(point);
    if (!index) {
        return;
    }
    SetFocus(hwnd());
    events_.on_select(rows_[*index].key);
    events_.on_context_menu(screen);
}

void PlaylistList::on_command(WPARAM wparam) const {
    if (HIWORD(wparam) == BN_CLICKED) {
        events_.on_command(static_cast<CommandId>(LOWORD(wparam)));
    }
}

}
