#include "core/library/WindowsNames.h"

#include <algorithm>
#include <array>
#include <cctype>

namespace westie::core {

namespace {

constexpr std::array<std::string_view, 4> ReservedNames = {"CON", "PRN", "AUX", "NUL"};
// Followed by a digit from 1 to 9
constexpr std::array<std::string_view, 2> ReservedPorts = {"COM", "LPT"};

bool equal_ignoring_case(std::string_view a, std::string_view b) {
    return std::ranges::equal(a, b, [](char x, char y) { return std::toupper(static_cast<unsigned char>(x)) == std::toupper(static_cast<unsigned char>(y)); });
}

bool is_reserved(std::string_view name) {
    // CON.txt opens the device too, and so does CON with spaces before the extension
    name = name.substr(0, name.find('.'));
    while (name.ends_with(' ')) {
        name.remove_suffix(1);
    }
    if (std::ranges::any_of(ReservedNames, [&](std::string_view reserved) { return equal_ignoring_case(name, reserved); })) {
        return true;
    }
    return name.size() == 4 && name[3] >= '1' && name[3] <= '9'
           && std::ranges::any_of(ReservedPorts, [&](std::string_view port) { return equal_ignoring_case(name.substr(0, 3), port); });
}

}

NameProblem windows_name_problem(std::string_view name) {
    if (name.ends_with(' ') || name.ends_with('.')) {
        return NameProblem::EndsInSpaceOrPeriod;
    }
    if (is_reserved(name)) {
        return NameProblem::ReservedName;
    }
    return NameProblem::None;
}

}
