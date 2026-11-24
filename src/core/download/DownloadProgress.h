#pragma once

#include <optional>
#include <string_view>

namespace westie::core {

std::optional<int> download_percent(std::string_view line);

}
