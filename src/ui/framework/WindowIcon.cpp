#include "ui/framework/WindowIcon.h"

#include <utility>

#include "platform/win32/Win32Error.h"
#include "ui/framework/Instance.h"

namespace westie::ui::framework {

WindowIcon::WindowIcon(int resource_id) noexcept : resource_id_(resource_id) {}

void WindowIcon::apply_to(HWND hwnd, Dpi dpi) {
    platform::win32::UniqueIcon big_icon = load(SM_CXICON, dpi);
    platform::win32::UniqueIcon small_icon = load(SM_CXSMICON, dpi);
    SendMessageW(hwnd, WM_SETICON, ICON_BIG, reinterpret_cast<LPARAM>(big_icon.get()));
    SendMessageW(hwnd, WM_SETICON, ICON_SMALL, reinterpret_cast<LPARAM>(small_icon.get()));
    big_ = std::move(big_icon);
    small_ = std::move(small_icon);
}

platform::win32::UniqueIcon WindowIcon::load(int size_metric, Dpi dpi) const {
    int size = GetSystemMetricsForDpi(size_metric, dpi.value());
    HANDLE icon = LoadImageW(module_instance(), MAKEINTRESOURCEW(resource_id_), IMAGE_ICON, size, size, LR_DEFAULTCOLOR);
    return platform::win32::UniqueIcon(static_cast<HICON>(platform::win32::check(icon, "LoadImageW")));
}

}
