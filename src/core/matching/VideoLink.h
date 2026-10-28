#pragma once

#include <optional>
#include <string>
#include <string_view>

namespace westie::core {

// The video id a pasted YouTube or YouTube Music link, or an id alone, names
std::optional<std::string> parse_video_link(std::string_view text);

}
