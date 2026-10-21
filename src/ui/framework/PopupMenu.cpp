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

}
