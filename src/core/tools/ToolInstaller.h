#pragma once

#include <filesystem>
#include <functional>
#include <stop_token>

#include "core/tools/InstalledTools.h"
#include "core/tools/ToolPackage.h"
#include "core/tools/ToolSource.h"
#include "core/tools/ToolStatus.h"
#include "platform/FileOps.h"
#include "platform/Hasher.h"
#include "platform/HttpClient.h"

namespace westie::core {

class ToolInstaller {
public:
    using Progress = std::function<void(ToolState state, int percent)>;

    ToolInstaller(platform::FileOps& files, platform::HttpClient& http, const platform::Hasher& hasher, std::filesystem::path folder);

    InstalledTool install(ToolPackage package, const ToolDownload& download, const Progress& report, const std::stop_token& stop);
    void clear_staging();

private:
    std::filesystem::path staging_folder() const;

    platform::FileOps& files_;
    platform::HttpClient& http_;
    const platform::Hasher& hasher_;
    std::filesystem::path folder_;
};

}
