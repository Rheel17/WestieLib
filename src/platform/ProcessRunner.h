#pragma once

#include <filesystem>
#include <functional>
#include <span>
#include <stop_token>
#include <string>
#include <string_view>

namespace westie::platform {

struct ProcessResult {
    int exit_code = 0;
    std::string output{};
    std::string errors{};
};

// Each line of stdout as it arrives, without its line break
using OutputLineHandler = std::function<void(std::string_view line)>;

class ProcessRunner {
public:
    virtual ~ProcessRunner() = default;

    virtual ProcessResult run(
            const std::filesystem::path& program, std::span<const std::string> arguments, const OutputLineHandler& on_output_line, const std::stop_token& stop
    ) const = 0;
};

}
