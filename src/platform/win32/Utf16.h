#pragma once

#include <string>
#include <string_view>

namespace westie::platform::win32 {

std::wstring to_utf16(std::string_view utf8);

}
