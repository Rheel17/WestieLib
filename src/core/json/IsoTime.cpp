#include "core/json/IsoTime.h"

#include <format>
#include <sstream>

namespace westie::core {

std::string format_iso_time(std::chrono::system_clock::time_point time) {
    return std::format("{:%FT%TZ}", std::chrono::floor<std::chrono::seconds>(time));
}

std::optional<std::chrono::system_clock::time_point> parse_iso_time(std::string_view text) {
    std::istringstream input{std::string(text)};
    std::chrono::sys_seconds time;
    input >> std::chrono::parse("%FT%TZ", time);
    if (!input) {
        return std::nullopt;
    }
    return time;
}

}
