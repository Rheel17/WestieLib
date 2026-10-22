#pragma once

#include <array>
#include <filesystem>
#include <mutex>
#include <shared_mutex>

#include "core/tools/ToolPackage.h"

namespace westie::core {

class ToolFolder {
public:
    explicit ToolFolder(std::filesystem::path path);

    const std::filesystem::path& path() const noexcept;
    std::filesystem::path program(ToolPackage package) const;
    std::shared_lock<std::shared_mutex> lock_for_use(ToolPackage package);
    std::unique_lock<std::shared_mutex> lock_for_replace(ToolPackage package);

private:
    std::filesystem::path path_;
    std::array<std::shared_mutex, AllToolPackages.size()> locks_;
};

}
