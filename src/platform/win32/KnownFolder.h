#pragma once

#include <filesystem>

#include <windows.h>
#include <shtypes.h>

namespace westie::platform::win32 {

std::filesystem::path known_folder(const KNOWNFOLDERID& id);

}
