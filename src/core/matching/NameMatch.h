#pragma once

#include <span>
#include <string>

namespace westie::core {

// Ordered from the closest
enum class NameMatch { Same, Starts, Inside, Other };

NameMatch name_match(std::span<const std::string> a, std::span<const std::string> b);

}
