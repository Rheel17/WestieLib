#pragma once

#include <filesystem>

#include "core/Settings.h"
#include "platform/FileOps.h"

namespace westie::core {

class SettingsStore {
public:
    SettingsStore(platform::FileOps& files, std::filesystem::path file, Settings defaults);

    const Settings& defaults() const noexcept;
    Settings load() const;
    void save(const Settings& settings);

private:
    platform::FileOps& files_;
    std::filesystem::path file_;
    Settings defaults_;
};

}
