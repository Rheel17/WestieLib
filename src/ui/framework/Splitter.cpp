#include "ui/framework/Splitter.h"

#include <utility>

#include <windowsx.h>

#include "ui/framework/WindowClass.h"

namespace westie::ui::framework {

Splitter::Splitter(HWND parent, SplitterAxis axis, std::function<void(int position)> on_drag) :
        Window(window_class(), {.style = WS_CHILD | WS_VISIBLE | WS_CLIPSIBLINGS, .parent = parent}), axis_(axis), on_drag_(std::move(on_drag)) {}

LRESULT Splitter::handle_message(UINT message, WPARAM wparam, LPARAM lparam) {
    switch (message) {
    case WM_SETCURSOR:      return on_set_cursor(lparam) ? TRUE : Window::handle_message(message, wparam, lparam);
    case WM_LBUTTONDOWN:    on_button_down(lparam); return 0;
    case WM_MOUSEMOVE:      on_mouse_move(wparam, lparam); return 0;
    case WM_LBUTTONUP:      on_button_up(); return 0;
    case WM_CAPTURECHANGED: grab_offset_.reset(); return 0;
    default:                return Window::handle_message(message, wparam, lparam);
    }
}

const WindowClass& Splitter::window_class() {
    // The cursor depends on the axis, so WM_SETCURSOR sets it
    static WindowClass window_class({.name = L"WestieLib.Splitter", .background = GetSysColorBrush(COLOR_BTNFACE), .cursor = nullptr});
    return window_class;
}

int Splitter::along_axis(LPARAM lparam) const noexcept {
    return axis_ == SplitterAxis::X ? GET_X_LPARAM(lparam) : GET_Y_LPARAM(lparam);
}

bool Splitter::on_set_cursor(LPARAM lparam) const {
    if (LOWORD(lparam) != HTCLIENT) {
        return false;
    }
    SetCursor(LoadCursorW(nullptr, axis_ == SplitterAxis::X ? IDC_SIZEWE : IDC_SIZENS));
    return true;
}

void Splitter::on_button_down(LPARAM lparam) {
    grab_offset_ = along_axis(lparam);
    SetCapture(hwnd());
}

void Splitter::on_mouse_move(WPARAM keys, LPARAM lparam) {
    // Windows also sends a move when a window moves under a cursor that stands still, with the button up, and the
    // capture brings that one here even from far away
    if (!grab_offset_ || (keys & MK_LBUTTON) == 0) {
        return;
    }
    POINT point{GET_X_LPARAM(lparam), GET_Y_LPARAM(lparam)};
    MapWindowPoints(hwnd(), GetParent(hwnd()), &point, 1);
    on_drag_((axis_ == SplitterAxis::X ? point.x : point.y) - *grab_offset_);
}

void Splitter::on_button_up() {
    grab_offset_.reset();
    ReleaseCapture();
}

}
