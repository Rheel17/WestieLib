#pragma once

#include <chrono>
#include <filesystem>
#include <span>
#include <string>
#include <string_view>

#include "core/log/LogLevel.h"
#include "core/log/LogLine.h"
#include "platform/ProcessRunner.h"

namespace westie::core {

std::string_view log_level_name(LogLevel level) noexcept;
std::string format_log_time(std::chrono::local_time<std::chrono::milliseconds> time);
std::filesystem::path log_file_name(std::chrono::local_time<std::chrono::milliseconds> time);
std::string log_file_line(const LogLine& line);
std::string command_line(std::string_view tool, std::span<const std::string> arguments);
std::string log_tool_run(
        std::chrono::local_time<std::chrono::milliseconds> time, std::string_view tool, std::span<const std::string> arguments,
        const platform::ProcessResult& result
);

}
