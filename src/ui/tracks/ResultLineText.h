#pragma once

#include <optional>
#include <string>
#include <string_view>

namespace westie::ui {

std::wstring result_line_text(int shown, int total, std::optional<std::wstring_view> playlist_name);

}
