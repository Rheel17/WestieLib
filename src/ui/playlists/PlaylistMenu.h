#pragma once

#include <optional>

#include <windows.h>

#include "ui/commands/CommandId.h"
#include "ui/commands/CommandState.h"

namespace westie::ui {

std::optional<CommandId> choose_playlist_command(HWND owner, POINT screen, const CommandState& state);

}
