#pragma once

#include <array>
#include <chrono>
#include <optional>
#include <span>

#include "core/tools/ToolPackage.h"
#include "core/tools/ToolStatus.h"
#include "ui/tools/ToolMark.h"

namespace westie::ui {

class ToolBoard {
public:
    ToolBoard();

    void apply(const core::ToolStatus& status);
    void set_running(bool running) noexcept;
    void set_checked_at(std::chrono::system_clock::time_point checked_at) noexcept;

    std::span<const core::ToolStatus> statuses() const noexcept;
    std::optional<std::chrono::system_clock::time_point> checked_at() const noexcept;
    bool running() const noexcept;
    bool available(core::ToolPackage package) const noexcept;
    ToolMark mark() const noexcept;

private:
    std::array<core::ToolStatus, core::AllToolPackages.size()> statuses_;
    std::optional<std::chrono::system_clock::time_point> checked_at_;
    bool running_ = false;
};

}
