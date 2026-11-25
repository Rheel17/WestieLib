#pragma once

#include <string>
#include <string_view>

namespace westie::platform::win32 {

std::wstring to_utf16(std::string_view utf8);
std::string to_utf8(std::wstring_view utf16);
bool is_utf8(std::string_view text);
std::string ansi_to_utf8(std::string_view ansi);

}
