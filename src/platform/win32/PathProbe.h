#pragma once

#include <filesystem>

namespace westie::platform::win32 {

enum class PathKind { Folder, File, Nothing, NoSuchDrive, Unknown };

PathKind probe_path(const std::filesystem::path& path);

}
