#include "ui/playlists/PlaylistList.h"

#include <algorithm>
#include <utility>

#include "ResourceIds.h"
#include "platform/win32/Win32Error.h"
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
constexpr int ScrollLineDips = 16;
constexpr UINT DefaultWheelScrollLines = 3;

}

PlaylistList::PlaylistList(HWND parent, std::function<void(CommandId)> on_command) :
        Window(window_class(), {.title = L"Playlist list", .style = WS_CHILD | WS_VISIBLE | WS_VSCROLL | WS_CLIPCHILDREN | WS_CLIPSIBLINGS, .parent = parent}),
        on_command_(std::move(on_command)),
        empty_text_(hwnd(), L"No playlists yet.", {.color = framework::TextColor::Gray}),
        add_button_(hwnd(), {.text = L"Add playlist", .command = std::to_underlying(CommandId::AddPlaylist), .icon = IDI_PLUS}) {}

void PlaylistList::apply_dpi(framework::Dpi dpi, const framework::FontSet& fonts) {
    painter_.emplace(dpi, fonts);
    empty_text_.set_font(fonts.regular().handle());
    empty_text_height_ = std::max(dpi.scale(EmptyTextLineDips), fonts.regular().line_height());
    add_button_.apply_dpi(dpi, fonts.regular());
    layout();
}

void PlaylistList::set_rows(std::vector<PlaylistRow> rows) {
    rows_ = std::move(rows);
    layout();
}

LRESULT PlaylistList::handle_message(UINT message, WPARAM wparam, LPARAM lparam) {
    switch (message) {
    case WM_SIZE:       layout(); return 0;
    // The paint fills the background, so the rows aren't erased first and don't blink
    case WM_ERASEBKGND: return TRUE;
    case WM_PAINT:      paint(); return 0;
    case WM_VSCROLL:    on_vertical_scroll(LOWORD(wparam)); return 0;
    case WM_MOUSEWHEEL: on_mouse_wheel(GET_WHEEL_DELTA_WPARAM(wparam)); return 0;
    case WM_COMMAND:    on_command(wparam); return 0;
    default:            return Window::handle_message(message, wparam, lparam);
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
    framework::Dpi dpi = framework::Dpi::of(hwnd());
    int top = dpi.scale(RowGapDips) + static_cast<int>(index) * (painter_->height() + dpi.scale(RowGapDips)) - scroll_;
    return {dpi.scale(RowInsetDips), top, client().right - dpi.scale(RowInsetDips), top + painter_->height()};
}

int PlaylistList::rows_bottom() const {
    if (rows_.empty()) {
        return 0;
    }
    return row_bounds(rows_.size() - 1).bottom + framework::Dpi::of(hwnd()).scale(RowGapDips) + scroll_;
}

int PlaylistList::content_height() const {
    framework::Dpi dpi = framework::Dpi::of(hwnd());
    int above_button = rows_.empty() ? dpi.scale(EmptyTextTopDips) + empty_text_height_ + dpi.scale(EmptyTextBottomDips) : rows_bottom();
    return above_button + dpi.scale(ButtonTopDips) + add_button_.size().height + dpi.scale(BottomPaddingDips);
}

void PlaylistList::layout() {
    if (!painter_) {
        return;
    }

    // Showing or hiding the scroll bar sends WM_SIZE, which lays out again before this goes on with the new width
    int page = client().height();
    scroll_ = std::clamp(scroll_, 0, std::max(0, content_height() - page));
    SCROLLINFO info{};
    info.cbSize = sizeof(info);
    info.fMask = SIF_RANGE | SIF_PAGE | SIF_POS;
    info.nMax = content_height() - 1;
    info.nPage = static_cast<UINT>(page);
    info.nPos = scroll_;
    SetScrollInfo(hwnd(), SB_VERT, &info, TRUE);

    framework::Dpi dpi = framework::Dpi::of(hwnd());
    framework::Rect area = client();
    // The empty list's text and the button line up with the playlist names, and so with All tracks above
    int text_left = dpi.scale(RowInsetDips) + painter_->text_left();
    int empty_top = dpi.scale(EmptyTextTopDips) - scroll_;
    framework::Rect empty{text_left, empty_top, area.right - dpi.scale(EmptyTextRightDips), empty_top + empty_text_height_};

    int button_top = (rows_.empty() ? empty.bottom + dpi.scale(EmptyTextBottomDips) : rows_bottom() - scroll_) + dpi.scale(ButtonTopDips);
    framework::Size button = add_button_.size();

    platform::win32::check(InvalidateRect(hwnd(), nullptr, FALSE), "InvalidateRect");
    framework::DeferredPositions positions;
    positions.place(empty_text_.hwnd(), rows_.empty() ? std::optional(empty) : std::nullopt);
    positions.place(add_button_.hwnd(), framework::Rect{text_left, button_top, text_left + button.width, button_top + button.height});
    positions.apply();
}

void PlaylistList::scroll_to(int position) {
    int clamped = std::clamp(position, 0, std::max(0, content_height() - client().height()));
    if (clamped != scroll_) {
        scroll_ = clamped;
        layout();
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
    for (std::size_t index = 0; index < rows_.size(); ++index) {
        framework::Rect bounds = row_bounds(index);
        if (bounds.bottom > area.top && bounds.top < area.bottom) {
            painter_->paint(paint.dc(), bounds, rows_[index]);
        }
    }
}

void PlaylistList::on_vertical_scroll(WORD request) {
    SCROLLINFO info{};
    info.cbSize = sizeof(info);
    info.fMask = SIF_ALL;
    platform::win32::check(GetScrollInfo(hwnd(), SB_VERT, &info), "GetScrollInfo");
    int line = framework::Dpi::of(hwnd()).scale(ScrollLineDips);
    int page = static_cast<int>(info.nPage);

    switch (request) {
    case SB_LINEUP:        scroll_to(scroll_ - line); return;
    case SB_LINEDOWN:      scroll_to(scroll_ + line); return;
    case SB_PAGEUP:        scroll_to(scroll_ - page); return;
    case SB_PAGEDOWN:      scroll_to(scroll_ + page); return;
    case SB_THUMBTRACK:
    case SB_THUMBPOSITION: scroll_to(info.nTrackPos); return;
    case SB_TOP:           scroll_to(0); return;
    case SB_BOTTOM:        scroll_to(info.nMax); return;
    default:               return;
    }
}

void PlaylistList::on_mouse_wheel(short delta) {
    UINT lines = DefaultWheelScrollLines;
    SystemParametersInfoW(SPI_GETWHEELSCROLLLINES, 0, &lines, 0);
    int page = client().height();
    int distance = lines == WHEEL_PAGESCROLL ? page : static_cast<int>(lines) * framework::Dpi::of(hwnd()).scale(ScrollLineDips);

    // A touchpad sends small deltas, which add up to a notch
    wheel_remainder_ += delta;
    int notches = wheel_remainder_ / WHEEL_DELTA;
    wheel_remainder_ %= WHEEL_DELTA;
    scroll_to(scroll_ - notches * distance);
}

void PlaylistList::on_command(WPARAM wparam) const {
    if (HIWORD(wparam) == BN_CLICKED) {
        on_command_(static_cast<CommandId>(LOWORD(wparam)));
    }
}

}
