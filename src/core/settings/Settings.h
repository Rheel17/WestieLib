#pragma once

#include <filesystem>

namespace westie::core {

struct Settings {
    std::filesystem::path library_path{};
    bool auto_update_tools = true;

    bool operator==(const Settings&) const = default;
};

}
