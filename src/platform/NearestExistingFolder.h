#pragma once

#include <filesystem>
#include <optional>

namespace westie::platform {

std::optional<std::filesystem::path> nearest_existing_folder(std::filesystem::path folder);

}
