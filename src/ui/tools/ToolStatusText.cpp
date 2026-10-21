#include "ui/tools/ToolStatusText.h"

#include <algorithm>
#include <format>

#include "platform/win32/LocalTime.h"
#include "platform/win32/Utf16.h"
#include "ui/RelativeDayText.h"

namespace westie::ui {

namespace {

constexpr int FullPercent = 100;

bool is_installing(const core::ToolStatus& status) {
    return status.state == core::ToolState::Downloading || status.state == core::ToolState::Installing;
}

}

std::wstring tool_name_text(core::ToolPackage package) {
    return platform::win32::to_utf16(core::tool_package_name(package));
}

std::wstring tool_state_text(const core::ToolStatus& status) {
    switch (status.state) {
    case core::ToolState::Missing:     return L"Not installed";
    case core::ToolState::Installed:   return L"Installed";
    case core::ToolState::Checking:    return L"Checking…";
    case core::ToolState::Downloading: return std::format(L"Downloading {}%", status.percent);
    case core::ToolState::Installing:  return L"Installing…";
    case core::ToolState::UpToDate:    return L"Up to date";
    case core::ToolState::Failed:      return L"Failed: " + platform::win32::to_utf16(status.error);
    }
    return {};
}

std::wstring tools_tooltip_text(const ToolBoard& board) {
    std::wstring text;
    for (const core::ToolStatus& status : board.statuses()) {
        if (!text.empty()) {
            text += L"\r\n";
        }
        text += tool_name_text(status.package);
        if (status.state == core::ToolState::Failed) {
            text += L": " + platform::win32::to_utf16(status.error);
        } else if (status.release.empty()) {
            text += L": not installed";
        } else {
            text += L" " + platform::win32::to_utf16(status.release);
        }
    }
    return text;
}

std::optional<ToolProgress> tools_progress(const ToolBoard& board) {
    if (!board.running()) {
        return std::nullopt;
    }

    auto statuses = board.statuses();
    if (auto installing = std::ranges::find_if(statuses, is_installing); installing != statuses.end()) {
        std::wstring work = installing->release.empty() ? L"Installing tools…" : L"Updating tools…";
        std::wstring name = tool_name_text(installing->package);
        if (installing->state == core::ToolState::Downloading) {
            return ToolProgress{.text = std::format(L"{} {} {}%", work, name, installing->percent), .percent = installing->percent};
        }
        return ToolProgress{.text = std::format(L"{} {}", work, name), .percent = FullPercent};
    }
    if (std::ranges::any_of(statuses, [](const core::ToolStatus& status) { return status.state == core::ToolState::Checking; })) {
        return ToolProgress{.text = L"Checking for tool updates…"};
    }
    return std::nullopt;
}

std::wstring last_checked_text(const ToolBoard& board, std::chrono::system_clock::time_point now) {
    if (!board.checked_at()) {
        return L"Never checked";
    }
    return L"Last checked " + relative_day_text(platform::win32::to_local_time(*board.checked_at()), platform::win32::to_local_time(now));
}

}
