#pragma once

#include <filesystem>
#include <mutex>

#include "core/settings/Settings.h"
#include "platform/FileOps.h"

namespace westie::core {

class SettingsStore {
public:
    SettingsStore(platform::FileOps& files, std::filesystem::path file, Settings defaults);

    const Settings& defaults() const noexcept;
    Settings load();
    void save(const Settings& settings);
    Settings current() const;

private:
    platform::FileOps& files_;
    std::filesystem::path file_;
    Settings defaults_;
    // The core's threads read what the UI saved last
    mutable std::mutex mutex_;
    Settings current_;
};

}
