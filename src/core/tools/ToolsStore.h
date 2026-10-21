#pragma once

#include <filesystem>

#include "core/tools/InstalledTools.h"
#include "platform/FileOps.h"

namespace westie::core {

class ToolsStore {
public:
    ToolsStore(platform::FileOps& files, std::filesystem::path file);

    InstalledTools load() const;
    void save(const InstalledTools& tools);

private:
    platform::FileOps& files_;
    std::filesystem::path file_;
};

}
