#pragma once

#include <chrono>
#include <optional>
#include <string>
#include <string_view>

namespace westie::core {

std::string format_iso_time(std::chrono::system_clock::time_point time);
std::optional<std::chrono::system_clock::time_point> parse_iso_time(std::string_view text);

}
