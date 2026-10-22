#include "ui/framework/VerticalScroll.h"

#include <algorithm>

#include "platform/win32/Win32Error.h"
#include "ui/framework/Dpi.h"

namespace westie::ui::framework {

namespace {

constexpr int LineDips = 16;
constexpr UINT DefaultWheelScrollLines = 3;

int page_of(HWND window) {
    RECT client;
    platform::win32::check(GetClientRect(window, &client), "GetClientRect");
    return client.bottom - client.top;
}

}

int VerticalScroll::position() const noexcept {
    return position_;
}

void VerticalScroll::update(HWND window, int content_height) {
    content_height_ = content_height;
    int page = page_of(window);
    position_ = std::clamp(position_, 0, std::max(0, content_height - page));

    // Showing or hiding the scroll bar sends WM_SIZE, so the owner lays out again before this returns
    SCROLLINFO info{};
    info.cbSize = sizeof(info);
    info.fMask = SIF_RANGE | SIF_PAGE | SIF_POS;
    info.nMax = content_height - 1;
    info.nPage = static_cast<UINT>(page);
    info.nPos = position_;
    SetScrollInfo(window, SB_VERT, &info, TRUE);
}

bool VerticalScroll::scroll_to(HWND window, int position) {
    int clamped = std::clamp(position, 0, std::max(0, content_height_ - page_of(window)));
    if (clamped == position_) {
        return false;
    }
    position_ = clamped;
    return true;
}

bool VerticalScroll::reveal(HWND window, int top, int bottom) {
    int page = page_of(window);
    if (top < position_ || bottom - top > page) {
        return scroll_to(window, top);
    }
    if (bottom > position_ + page) {
        return scroll_to(window, bottom - page);
    }
    return false;
}

bool VerticalScroll::on_scroll(HWND window, WORD request) {
    SCROLLINFO info{};
    info.cbSize = sizeof(info);
    info.fMask = SIF_ALL;
    platform::win32::check(GetScrollInfo(window, SB_VERT, &info), "GetScrollInfo");
    int line = Dpi::of(window).scale(LineDips);
    int page = static_cast<int>(info.nPage);

    switch (request) {
    case SB_LINEUP:        return scroll_to(window, position_ - line);
    case SB_LINEDOWN:      return scroll_to(window, position_ + line);
    case SB_PAGEUP:        return scroll_to(window, position_ - page);
    case SB_PAGEDOWN:      return scroll_to(window, position_ + page);
    case SB_THUMBTRACK:
    case SB_THUMBPOSITION: return scroll_to(window, info.nTrackPos);
    case SB_TOP:           return scroll_to(window, 0);
    case SB_BOTTOM:        return scroll_to(window, info.nMax);
    default:               return false;
    }
}

bool VerticalScroll::on_wheel(HWND window, short delta) {
    UINT lines = DefaultWheelScrollLines;
    SystemParametersInfoW(SPI_GETWHEELSCROLLLINES, 0, &lines, 0);
    int distance = lines == WHEEL_PAGESCROLL ? page_of(window) : static_cast<int>(lines) * Dpi::of(window).scale(LineDips);

    // A touchpad sends small deltas, which add up to a notch
    wheel_remainder_ += delta;
    int notches = wheel_remainder_ / WHEEL_DELTA;
    wheel_remainder_ %= WHEEL_DELTA;
    return scroll_to(window, position_ - notches * distance);
}

}
