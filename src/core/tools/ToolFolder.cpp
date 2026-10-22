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

}
