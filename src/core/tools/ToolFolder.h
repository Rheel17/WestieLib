#pragma once

#include <array>
#include <condition_variable>
#include <cstdint>
#include <filesystem>
#include <mutex>
#include <shared_mutex>
#include <stop_token>

#include "core/tools/ToolPackage.h"

namespace westie::core {

class ToolFolder {
public:
    explicit ToolFolder(std::filesystem::path path);

    const std::filesystem::path& path() const noexcept;
    std::filesystem::path program(ToolPackage package) const;
    std::shared_lock<std::shared_mutex> lock_for_use(ToolPackage package);
    std::unique_lock<std::shared_mutex> lock_for_replace(ToolPackage package);
    std::uint64_t installs(ToolPackage package);
    bool wait_for_install_after(ToolPackage package, std::uint64_t seen, const std::stop_token& stop);
    void notify_installed(ToolPackage package);

private:
    std::filesystem::path path_;
    std::array<std::shared_mutex, AllToolPackages.size()> locks_;
    std::mutex installs_mutex_;
    std::condition_variable_any installed_;
    std::array<std::uint64_t, AllToolPackages.size()> installs_{};
};

}
