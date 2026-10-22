#pragma once

#include <filesystem>
#include <span>
#include <string>

namespace westie::platform::win32 {

std::wstring command_line(const std::filesystem::path& program, std::span<const std::string> arguments);

}
