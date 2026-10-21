#pragma once

#include <filesystem>

namespace westie::core {

struct Settings {
    std::filesystem::path library_path{};
};

}
