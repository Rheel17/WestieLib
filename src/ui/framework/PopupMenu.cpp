#include "ui/framework/PopupMenu.h"

#include "platform/win32/Win32Error.h"

namespace westie::ui::framework {

platform::win32::UniqueMenu build_popup_menu(std::span<const MenuItem> items) {
    platform::win32::UniqueMenu menu(platform::win32::check(CreatePopupMenu(), "CreatePopupMenu"));
    for (const MenuItem& item : items) {
        if (item.text) {
            platform::win32::check(AppendMenuW(menu.get(), MF_STRING, item.command, item.text), "AppendMenuW");
        } else {
            platform::win32::check(AppendMenuW(menu.get(), MF_SEPARATOR, 0, nullptr), "AppendMenuW");
        }
    }
    return menu;
}

std::optional<WORD> choose_from_popup(HWND owner, POINT screen, HMENU menu) {
    UINT flags = TPM_RETURNCMD | TPM_RIGHTBUTTON | TPM_NONOTIFY | (GetSystemMetrics(SM_MENUDROPALIGNMENT) != 0 ? TPM_RIGHTALIGN : TPM_LEFTALIGN);
    // With TPM_RETURNCMD the result is the chosen command, or 0 when the menu closes without one
    auto chosen = static_cast<WORD>(TrackPopupMenu(menu, flags, screen.x, screen.y, 0, owner, nullptr));
    return chosen == 0 ? std::nullopt : std::optional(chosen);
}

}
