#pragma once

#include <functional>
#include <optional>
#include <string>

#include <windows.h>

#include "ui/commands/CommandId.h"

namespace westie::ui {

struct PlaylistEvents {
    std::function<void(CommandId)> on_command{};
    // A playlist's key, or nothing for All tracks
    std::function<void(std::optional<std::string>)> on_select{};
    // Where the selected playlist's context menu goes, in screen coordinates
    std::function<void(POINT)> on_context_menu{};
    // A row's sync control was clicked
    std::function<void(const std::string&)> on_sync{};
};

}
