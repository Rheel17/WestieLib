#pragma once

#include <optional>
#include <span>
#include <utility>

#include <windows.h>

#include "ui/commands/CommandId.h"
#include "ui/commands/CommandState.h"
#include "ui/framework/PopupMenu.h"

namespace westie::ui {

constexpr framework::MenuItem command_item(CommandId command, const wchar_t* text) {
    return {.command = std::to_underlying(command), .text = text};
}

void update_menu_items(HMENU menu, const CommandState& state);
// A context menu of these commands, enabled as the state says
std::optional<CommandId> choose_command(HWND owner, POINT screen, std::span<const framework::MenuItem> items, const CommandState& state);

}
