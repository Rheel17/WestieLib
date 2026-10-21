#pragma once

#include <chrono>
#include <optional>
#include <string>

#include "core/tools/ToolStatus.h"
#include "ui/tools/ToolBoard.h"

namespace westie::ui {

struct ToolProgress {
    std::wstring text{};
    // Without a percentage the work has no bar
    std::optional<int> percent{};
};

std::wstring tool_name_text(core::ToolPackage package);
std::wstring tool_state_text(const core::ToolStatus& status);
std::wstring tools_tooltip_text(const ToolBoard& board);
std::optional<ToolProgress> tools_progress(const ToolBoard& board);
std::wstring last_checked_text(const ToolBoard& board, std::chrono::system_clock::time_point now);

}
