#pragma once

#include <filesystem>
#include <stop_token>
#include <thread>

#include "core/events/EventSink.h"
#include "core/tools/InstalledTools.h"
#include "core/tools/ToolPackage.h"
#include "core/tools/ToolStatus.h"
#include "core/tools/ToolsStore.h"
#include "platform/FileOps.h"

namespace westie::core {

class ToolManager {
public:
    ToolManager(platform::FileOps& files, std::filesystem::path folder, EventSink& events);

    void run(bool update_installed);
    void stop() noexcept;

private:
    void work(std::stop_token stop);
    void check_installed(const std::stop_token& stop);
    void report_failure(const char* error);
    ToolStatus installed_status(ToolPackage package, const InstalledTools& installed) const;

    platform::FileOps& files_;
    std::filesystem::path folder_;
    ToolsStore store_;
    EventSink& events_;
    // Last, so the thread is joined before the members it uses go
    std::jthread worker_;
};

}
