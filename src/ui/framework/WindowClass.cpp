#include "ui/framework/WindowClass.h"

#include <utility>

#include "platform/win32/Win32Error.h"
#include "ui/framework/Instance.h"
#include "ui/framework/Window.h"

namespace westie::ui::framework {

WindowClass::WindowClass(Options options) : name_(std::move(options.name)) {
    WNDCLASSEXW window_class{
        .cbSize = sizeof(WNDCLASSEXW),
        .style = options.style,
        .lpfnWndProc = Window::window_proc,
        .cbClsExtra = 0,
        .cbWndExtra = sizeof(Window*),
        .hInstance = module_instance(),
        .hIcon = options.icon,
        .hCursor = options.cursor,
        .hbrBackground = options.background,
        .lpszMenuName = nullptr,
        .lpszClassName = name_.c_str(),
        .hIconSm = nullptr,
    };
    platform::win32::check(RegisterClassExW(&window_class), "RegisterClassExW");
}

WindowClass::~WindowClass() {
    UnregisterClassW(name_.c_str(), module_instance());
}

const std::wstring& WindowClass::name() const noexcept {
    return name_;
}

}
