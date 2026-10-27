#pragma once

#include <cstddef>
#include <string_view>

namespace westie::ui::framework {

std::size_t word_start_before(std::wstring_view text, std::size_t caret);
std::size_t word_end_after(std::wstring_view text, std::size_t caret);

}
