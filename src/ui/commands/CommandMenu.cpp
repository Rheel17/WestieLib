#include "ui/commands/CommandMenu.h"

#include "platform/win32/UniqueHandle.h"

namespace westie::ui {

void update_menu_items(HMENU menu, const CommandState& state) {
    int count = GetMenuItemCount(menu);
    for (int position = 0; position < count; position++) {
        UINT id = GetMenuItemID(menu, position);
        // Separators have ID 0, submenus -1
        if (id == 0 || id == static_cast<UINT>(-1)) {
            continue;
        }

        auto command = static_cast<CommandId>(id);
        UINT by_position = static_cast<UINT>(position);
        EnableMenuItem(menu, by_position, MF_BYPOSITION | (state.enabled(command) ? MF_ENABLED : MF_GRAYED));
        CheckMenuItem(menu, by_position, MF_BYPOSITION | (state.checked(command) ? MF_CHECKED : MF_UNCHECKED));
    }
}

std::optional<CommandId> choose_command(HWND owner, POINT screen, std::span<const framework::MenuItem> items, const CommandState& state) {
    platform::win32::UniqueMenu menu = framework::build_popup_menu(items);
    update_menu_items(menu.get(), state);
    std::optional<WORD> chosen = framework::choose_from_popup(owner, screen, menu.get());
    return chosen ? std::optional(static_cast<CommandId>(*chosen)) : std::nullopt;
}

}
