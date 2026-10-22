#include "core/tools/ToolManager.h"

#include <algorithm>
#include <chrono>
#include <exception>
#include <format>
#include <optional>
#include <stdexcept>
#include <utility>

#include "core/github/Release.h"
#include "core/tools/ToolSource.h"
#include "core/tools/ToolsChecked.h"
#include "core/tools/ToolsRunChanged.h"
#include "platform/Cancelled.h"

namespace westie::core {

ToolManager::ToolManager(ToolServices services, std::string github_api_url, std::filesystem::path folder, EventSink& events) :
        files_(services.files),
        folder_(std::move(folder)),
        store_(services.files, folder_ / L"tools.json"),
        releases_(services.http, std::move(github_api_url)),
        installer_(services, folder_),
        events_(events) {}

void ToolManager::run(bool update_installed) {
    // Emitted here rather than on the worker, so the UI knows of the run before anything else it reports
    events_.emit(ToolsRunChanged{.running = true});
    worker_ = std::jthread([this, update_installed](std::stop_token stop) { work(stop, update_installed); });
}

void ToolManager::stop() noexcept {
    worker_.request_stop();
}

void ToolManager::work(const std::stop_token& stop, bool update_installed) {
    // An exception that leaves a thread ends the process, so it becomes every tool's failure instead
    try {
        update_tools(stop, update_installed);
    } catch (const platform::Cancelled&) {
        // Only the app's close stops a run, and nothing waits for its result
    } catch (const std::exception& error) {
        report_failure(error.what());
    }
    events_.emit(ToolsRunChanged{.running = false});
}

void ToolManager::update_tools(const std::stop_token& stop, bool update_installed) {
    InstalledTools installed = store_.load();
    std::optional<std::chrono::system_clock::time_point> checked_at = installed.checked_at;
    if (checked_at) {
        events_.emit(ToolsChecked{.checked_at = *checked_at});
    }
    // Left behind by a run that didn't finish
    installer_.clear_staging();

    for (ToolPackage package : AllToolPackages) {
        events_.emit(installed_status(package, installed));
    }
    for (ToolPackage package : AllToolPackages) {
        ToolStatus status = installed_status(package, installed);
        if (status.state == ToolState::Installed && !update_installed) {
            continue;
        }
        try {
            update_tool(package, status.release, installed, stop);
        } catch (const platform::Cancelled&) {
            throw;
        } catch (const std::exception& error) {
            events_.emit(ToolStatus{.package = package, .state = ToolState::Failed, .release = status.release, .error = error.what()});
        }
    }

    installer_.clear_staging();
    // A look-up that went well counts as a check, even when the install after it failed
    if (installed.checked_at != checked_at) {
        store_.save(installed);
        events_.emit(ToolsChecked{.checked_at = *installed.checked_at});
    }
}

void ToolManager::update_tool(ToolPackage package, const std::string& current, InstalledTools& installed, const std::stop_token& stop) {
    events_.emit(ToolStatus{.package = package, .state = ToolState::Checking, .release = current});
    std::string_view repository = tool_repository(package);
    Release release = releases_.latest(repository, stop);
    installed.checked_at = std::chrono::system_clock::now();

    std::optional<ToolDownload> download = choose_download(package, release);
    if (!download) {
        throw std::runtime_error(std::format("{}'s release {} has no download for Windows", repository, release.tag));
    }

    // A missing tool has no current release, so it always installs
    if (download->release != current) {
        auto report = [&](ToolState state, int percent) {
            events_.emit(ToolStatus{.package = package, .state = state, .release = current, .percent = percent});
        };
        installed.tools[package] = installer_.install(package, *download, report, stop);
        store_.save(installed);
    }
    events_.emit(ToolStatus{.package = package, .state = ToolState::UpToDate, .release = installed.tools[package].release});
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
