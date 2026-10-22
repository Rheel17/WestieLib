#include "ui/tools/ToolBoard.h"

#include <algorithm>
#include <cstddef>
#include <utility>

namespace westie::ui {

ToolBoard::ToolBoard() {
    for (std::size_t index = 0; index < statuses_.size(); ++index) {
        statuses_[index].package = core::AllToolPackages[index];
    }
}

void ToolBoard::apply(const core::ToolStatus& status) {
    statuses_[std::to_underlying(status.package)] = status;
}

void ToolBoard::set_running(bool running) noexcept {
    running_ = running;
}

void ToolBoard::set_checked_at(std::chrono::system_clock::time_point checked_at) noexcept {
    checked_at_ = checked_at;
}

std::span<const core::ToolStatus> ToolBoard::statuses() const noexcept {
    return statuses_;
}

std::optional<std::chrono::system_clock::time_point> ToolBoard::checked_at() const noexcept {
    return checked_at_;
}

bool ToolBoard::running() const noexcept {
    return running_;
}

bool ToolBoard::available(core::ToolPackage package) const noexcept {
    // A failed update leaves the release before it in place, and so does an update that is going on
    return !statuses_[std::to_underlying(package)].release.empty();
}

ToolMark ToolBoard::mark() const noexcept {
    if (running_) {
        return ToolMark::Working;
    }
    bool attention = std::ranges::any_of(statuses_, [](const core::ToolStatus& status) { return tool_mark(status) == ToolMark::Attention; });
    return attention ? ToolMark::Attention : ToolMark::Ready;
}

}
