#include "core/tools/ToolFolder.h"

#include <utility>

namespace westie::core {

ToolFolder::ToolFolder(std::filesystem::path path) : path_(std::move(path)) {}

const std::filesystem::path& ToolFolder::path() const noexcept {
    return path_;
}

std::filesystem::path ToolFolder::program(ToolPackage package) const {
    return path_ / tool_package_files(package).front();
}

std::shared_lock<std::shared_mutex> ToolFolder::lock_for_use(ToolPackage package) {
    return std::shared_lock(locks_[std::to_underlying(package)]);
}

std::unique_lock<std::shared_mutex> ToolFolder::lock_for_replace(ToolPackage package) {
    return std::unique_lock(locks_[std::to_underlying(package)]);
}

std::uint64_t ToolFolder::installs(ToolPackage package) {
    std::scoped_lock lock(installs_mutex_);
    return installs_[std::to_underlying(package)];
}

bool ToolFolder::wait_for_install_after(ToolPackage package, std::uint64_t seen, const std::stop_token& stop) {
    std::unique_lock lock(installs_mutex_);
    // Counting installs, not waiting for the next one, catches one that came between the caller's try and this wait
    return installed_.wait(lock, stop, [&] { return installs_[std::to_underlying(package)] > seen; });
}

void ToolFolder::notify_installed(ToolPackage package) {
    {
        std::scoped_lock lock(installs_mutex_);
        ++installs_[std::to_underlying(package)];
    }
    installed_.notify_all();
}

}
