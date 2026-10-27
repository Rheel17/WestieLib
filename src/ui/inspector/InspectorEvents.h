#pragma once

#include <functional>
#include <string>

#include "ui/commands/CommandId.h"

namespace westie::ui {

struct InspectorEvents {
    std::function<void(CommandId)> on_command{};
    std::function<void(const std::wstring&)> on_open_url{};
    // A playlist's key
    std::function<void(const std::string&)> on_select_playlist{};
};

}
