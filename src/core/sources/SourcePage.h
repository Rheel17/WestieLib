#pragma once

#include <optional>
#include <string>
#include <string_view>

namespace westie::core {

std::optional<std::string> source_page_url(std::string_view key);

}
