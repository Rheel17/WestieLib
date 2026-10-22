#pragma once

#include <functional>
#include <optional>
#include <string>

#include "ui/commands/CommandId.h"

namespace westie::ui {

struct PlaylistEvents {
    std::function<void(CommandId)> on_command{};
    // A playlist's key, or nothing for All tracks
    std::function<void(std::optional<std::string>)> on_select{};
};

}
