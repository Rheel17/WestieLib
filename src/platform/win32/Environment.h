#pragma once

#include <optional>
#include <string>

namespace westie::platform::win32 {

std::optional<std::wstring> environment_variable(const wchar_t* name);

}
