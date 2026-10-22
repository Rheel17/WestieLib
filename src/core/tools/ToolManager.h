#pragma once

#include <filesystem>
#include <stop_token>
#include <string>
#include <thread>

#include "core/events/EventSink.h"
#include "core/github/GitHubReleases.h"
#include "core/tools/InstalledTools.h"
#include "core/tools/ToolInstaller.h"
#include "core/tools/ToolPackage.h"
#include "core/tools/ToolServices.h"
#include "core/tools/ToolStatus.h"
#include "core/tools/ToolsStore.h"
#include "platform/FileOps.h"

namespace westie::core {

class ToolManager {
public:
    ToolManager(ToolServices services, std::string github_api_url, std::filesystem::path folder, EventSink& events);

    void run(bool update_installed);
    void stop() noexcept;

private:
    void work(const std::stop_token& stop, bool update_installed);
    void update_tools(const std::stop_token& stop, bool update_installed);
    void update_tool(ToolPackage package, const std::string& current, InstalledTools& installed, const std::stop_token& stop);
    void report_failure(const char* error);
    ToolStatus installed_status(ToolPackage package, const InstalledTools& installed) const;

    platform::FileOps& files_;
    std::filesystem::path folder_;
    ToolsStore store_;
    GitHubReleases releases_;
    ToolInstaller installer_;
    EventSink& events_;
    // Last, so the thread is joined before the members it uses go
    std::jthread worker_;
};

}
