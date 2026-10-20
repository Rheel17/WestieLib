#include "ui/framework/Splitter.h"

#include <utility>

#include <windowsx.h>

#include "ui/framework/WindowClass.h"

namespace westie::ui::framework {

Splitter::Splitter(HWND parent, std::function<void(int left)> on_drag) :
        Window(window_class(), {.style = WS_CHILD | WS_VISIBLE | WS_CLIPSIBLINGS, .parent = parent}), on_drag_(std::move(on_drag)) {}

LRESULT Splitter::handle_message(UINT message, WPARAM wparam, LPARAM lparam) {
    switch (message) {
    case WM_LBUTTONDOWN:    on_button_down(lparam); return 0;
    case WM_MOUSEMOVE:      on_mouse_move(lparam); return 0;
    case WM_LBUTTONUP:      on_button_up(); return 0;
    case WM_CAPTURECHANGED: grab_offset_.reset(); return 0;
    default:                return Window::handle_message(message, wparam, lparam);
    }
}

const WindowClass& Splitter::window_class() {
    static WindowClass window_class({
        .name = L"WestieLib.Splitter",
        .background = GetSysColorBrush(COLOR_BTNFACE),
        .cursor = LoadCursorW(nullptr, IDC_SIZEWE),
    });
    return window_class;
}

void Splitter::on_button_down(LPARAM lparam) {
    grab_offset_ = GET_X_LPARAM(lparam);
    SetCapture(hwnd());
}

void Splitter::on_mouse_move(LPARAM lparam) {
    if (!grab_offset_) {
        return;
    }
    POINT point{GET_X_LPARAM(lparam), GET_Y_LPARAM(lparam)};
    MapWindowPoints(hwnd(), GetParent(hwnd()), &point, 1);
    on_drag_(point.x - *grab_offset_);
}

void Splitter::on_button_up() {
    grab_offset_.reset();
    ReleaseCapture();
}

}
