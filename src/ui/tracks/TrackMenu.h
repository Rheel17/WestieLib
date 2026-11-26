#pragma once

#include <optional>
#include <span>

#include <windows.h>

#include "ui/commands/CommandId.h"
#include "ui/commands/CommandState.h"
#include "ui/framework/PopupMenu.h"

namespace westie::ui {

// The menu bar's Track menu, which is also a row's context menu
std::span<const framework::MenuItem> track_menu_items();
std::optional<CommandId> choose_track_command(HWND owner, POINT screen, const CommandState& state);

}
