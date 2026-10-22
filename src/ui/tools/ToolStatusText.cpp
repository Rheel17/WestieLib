#include "ui/tools/ToolStatusText.h"

#include <algorithm>
#include <format>

#include "platform/win32/LocalTime.h"
#include "platform/win32/Utf16.h"
#include "ui/RelativeDayText.h"

namespace westie::ui {

namespace {

constexpr int FullPercent = 100;

constexpr const wchar_t* CheckingText = L"checking";
constexpr const wchar_t* CheckingForUpdatesText = L"checking for updates";

bool is_installing(const core::ToolStatus& status) {
    return status.state == core::ToolState::Downloading || status.state == core::ToolState::Installing;
}

std::wstring install_text(bool update, core::ToolPackage package) {
    return std::format(L"{} {}", update ? L"updating" : L"installing", tool_name_text(package));
}

std::wstring download_text(bool update, core::ToolPackage package, int percent) {
    return std::format(L"{} {}%", install_text(update, package), percent);
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

std::optional<ToolWork> tools_work(const ToolBoard& board) {
    if (!board.running()) {
        return std::nullopt;
    }

    auto statuses = board.statuses();
    if (auto installing = std::ranges::find_if(statuses, is_installing); installing != statuses.end()) {
        bool update = !installing->release.empty();
        if (installing->state != core::ToolState::Downloading) {
            return ToolWork{.text = install_text(update, installing->package), .percent = FullPercent};
        }
        return ToolWork{.text = download_text(update, installing->package, installing->percent), .percent = installing->percent};
    }
    if (auto checking = std::ranges::find(statuses, core::ToolState::Checking, &core::ToolStatus::state); checking != statuses.end()) {
        // A missing tool's release is looked up to install it, not to update it
        return ToolWork{.text = checking->release.empty() ? install_text(false, checking->package) : CheckingForUpdatesText};
    }
    // Between the steps of a run, as when it looks at what is installed
    return ToolWork{.text = CheckingText};
}

std::vector<std::wstring> every_tools_work_text() {
    std::vector<std::wstring> texts{CheckingText, CheckingForUpdatesText};
    for (core::ToolPackage package : core::AllToolPackages) {
        texts.push_back(download_text(false, package, FullPercent));
        texts.push_back(download_text(true, package, FullPercent));
    }
    return texts;
}

std::wstring last_checked_text(const ToolBoard& board, std::chrono::system_clock::time_point now) {
    if (!board.checked_at()) {
        return L"Never checked";
    }
    return L"Last checked " + relative_day_text(platform::win32::to_local_time(*board.checked_at()), platform::win32::to_local_time(now));
}

}
