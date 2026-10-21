#include "core/tools/ToolManager.h"

#include <algorithm>
#include <exception>
#include <utility>

#include "core/tools/ToolsChecked.h"
#include "core/tools/ToolsRunChanged.h"

namespace westie::core {

ToolManager::ToolManager(platform::FileOps& files, std::filesystem::path folder, EventSink& events) :
        files_(files), folder_(std::move(folder)), store_(files, folder_ / L"tools.json"), events_(events) {}

void ToolManager::run([[maybe_unused]] bool update_installed) {
    // Emitted here rather than on the worker, so the UI knows of the run before anything else it reports
    events_.emit(ToolsRunChanged{.running = true});
    worker_ = std::jthread([this](std::stop_token stop) { work(std::move(stop)); });
}

void ToolManager::stop() noexcept {
    worker_.request_stop();
}

void ToolManager::work(std::stop_token stop) {
    // An exception that leaves a thread ends the process, so it becomes every tool's failure instead
    try {
        check_installed(stop);
    } catch (const std::exception& error) {
        report_failure(error.what());
    }
    events_.emit(ToolsRunChanged{.running = false});
}

void ToolManager::check_installed(const std::stop_token& stop) {
    InstalledTools installed = store_.load();
    if (installed.checked_at) {
        events_.emit(ToolsChecked{.checked_at = *installed.checked_at});
    }
    for (ToolPackage package : AllToolPackages) {
        if (stop.stop_requested()) {
            return;
        }
        events_.emit(installed_status(package, installed));
    }
}

void ToolManager::report_failure(const char* error) {
    for (ToolPackage package : AllToolPackages) {
        events_.emit(ToolStatus{.package = package, .state = ToolState::Failed, .error = error});
    }
}

ToolStatus ToolManager::installed_status(ToolPackage package, const InstalledTools& installed) const {
    auto tool = installed.tools.find(package);
    bool present = tool != installed.tools.end()
                   && std::ranges::all_of(tool_package_files(package), [this](std::wstring_view file) { return files_.is_file(folder_ / file); });
    if (!present) {
        return {.package = package, .state = ToolState::Missing};
    }
    return {.package = package, .state = ToolState::Installed, .release = tool->second.release};
}

}
