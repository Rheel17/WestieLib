#pragma once

#include <optional>
#include <string>
#include <string_view>

namespace westie::core {

// The YouTube Music id a pasted YouTube or YouTube Music link, or an id alone, names
std::optional<std::string> parse_ytm_link(std::string_view text);

}
