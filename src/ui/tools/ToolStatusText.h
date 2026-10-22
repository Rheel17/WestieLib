#pragma once

#include <chrono>
#include <optional>
#include <string>
#include <vector>

#include "core/tools/ToolStatus.h"
#include "ui/tools/ToolBoard.h"

namespace westie::ui {

struct ToolWork {
    std::wstring text{};
    // How full the progress ring is
    int percent = 0;
};

std::wstring tool_name_text(core::ToolPackage package);
std::wstring tool_state_text(const core::ToolStatus& status);
std::wstring tools_tooltip_text(const ToolBoard& board);
std::optional<ToolWork> tools_work(const ToolBoard& board);
std::vector<std::wstring> every_tools_work_text();
std::wstring last_checked_text(const ToolBoard& board, std::chrono::system_clock::time_point now);

}
