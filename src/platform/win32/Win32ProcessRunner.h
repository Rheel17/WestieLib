#pragma once

#include <filesystem>
#include <span>
#include <stop_token>
#include <string>

#include "platform/ProcessRunner.h"

namespace westie::platform::win32 {

class Win32ProcessRunner final : public ProcessRunner {
public:
    ProcessResult run(const std::filesystem::path& program, std::span<const std::string> arguments, const std::stop_token& stop) const override;
};

}
