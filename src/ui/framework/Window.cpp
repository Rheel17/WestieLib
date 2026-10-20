#include "ui/framework/Window.h"

#include <utility>

#include "platform/win32/Win32Error.h"
#include "ui/framework/Instance.h"
#include "ui/framework/PendingException.h"
#include "ui/framework/WindowClass.h"

namespace westie::ui::framework {

namespace {

constexpr int WindowPointerSlot = 0;

LRESULT failure_result(UINT message) noexcept {
    switch (message) {
    case WM_NCCREATE: return FALSE;
    case WM_CREATE:   return -1;
    default:          return 0;
    }
}

}

Window::Window(const WindowClass& window_class, const CreateParams& params) {
    HWND hwnd = CreateWindowExW(
            params.ex_style, window_class.name().c_str(), params.title.c_str(), params.style, CW_USEDEFAULT, CW_USEDEFAULT, CW_USEDEFAULT, CW_USEDEFAULT,
            params.parent, nullptr, module_instance(), this
    );

    // The destructor doesn't run for a constructor that throws, so a window that was created must go here
    try {
        PendingException::rethrow_if_any();
        platform::win32::check(hwnd, "CreateWindowExW");
    } catch (...) {
        destroy();
        throw;
    }
}

Window::~Window() {
    destroy();
}

HWND Window::hwnd() const noexcept {
    return hwnd_;
}

void Window::show(int show_command) {
    ShowWindow(hwnd_, show_command);
    UpdateWindow(hwnd_);
}

LRESULT Window::handle_message(UINT message, WPARAM wparam, LPARAM lparam) {
    return DefWindowProcW(hwnd_, message, wparam, lparam);
}

LRESULT CALLBACK Window::window_proc(HWND hwnd, UINT message, WPARAM wparam, LPARAM lparam) {
    if (message == WM_NCCREATE) {
        auto* window = static_cast<Window*>(reinterpret_cast<const CREATESTRUCTW*>(lparam)->lpCreateParams);
        window->hwnd_ = hwnd;
        SetWindowLongPtrW(hwnd, WindowPointerSlot, reinterpret_cast<LONG_PTR>(window));
    }

    auto* window = reinterpret_cast<Window*>(GetWindowLongPtrW(hwnd, WindowPointerSlot));
    if (!window) {
        return DefWindowProcW(hwnd, message, wparam, lparam);
    }

    LRESULT result;
    try {
        result = window->handle_message(message, wparam, lparam);
    } catch (...) {
        PendingException::capture();
        result = failure_result(message);
    }

    if (message == WM_NCDESTROY) {
        SetWindowLongPtrW(hwnd, WindowPointerSlot, 0);
        window->hwnd_ = nullptr;
    }
    return result;
}

void Window::destroy() noexcept {
    if (HWND hwnd = std::exchange(hwnd_, nullptr)) {
        SetWindowLongPtrW(hwnd, WindowPointerSlot, 0);
        DestroyWindow(hwnd);
    }
}

}
