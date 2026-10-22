#include "platform/NearestExistingFolder.h"

#include <system_error>

namespace westie::platform {

std::optional<std::filesystem::path> nearest_existing_folder(std::filesystem::path folder) {
    std::error_code error;
    while (!std::filesystem::is_directory(folder, error)) {
        if (!folder.has_relative_path()) {
            return std::nullopt;
        }
        folder = folder.parent_path();
    }
    return folder;
}

}
