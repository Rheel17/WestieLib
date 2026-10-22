#pragma once

#include <filesystem>
#include <functional>
#include <stop_token>

#include "core/github/Release.h"
#include "core/tools/InstalledTools.h"
#include "core/tools/ToolFolder.h"
#include "core/tools/ToolPackage.h"
#include "core/tools/ToolServices.h"
#include "core/tools/ToolSource.h"
#include "core/tools/ToolStatus.h"

namespace westie::core {

class ToolInstaller {
public:
    using Progress = std::function<void(ToolState state, int percent)>;

    ToolInstaller(const ToolServices& services, ToolFolder& folder);

    InstalledTool install(ToolPackage package, const ToolDownload& download, const Progress& report, const std::stop_token& stop);
    void clear_staging();

private:
    void fetch(const ReleaseAsset& asset, const std::filesystem::path& file, const Progress& report, const std::stop_token& stop);
    std::filesystem::path staging_folder() const;

    ToolServices services_;
    ToolFolder& folder_;
};

}
