#pragma once

#include <filesystem>
#include <span>
#include <stop_token>
#include <string>

namespace westie::platform {

struct ProcessResult {
    int exit_code = 0;
    std::string output{};
    std::string errors{};
};

class ProcessRunner {
public:
    virtual ~ProcessRunner() = default;

    virtual ProcessResult run(const std::filesystem::path& program, std::span<const std::string> arguments, const std::stop_token& stop) const = 0;
};

}
