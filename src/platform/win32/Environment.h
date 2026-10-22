#pragma once

#include <optional>
#include <span>
#include <string>
#include <string_view>

namespace westie::platform::win32 {

struct EnvironmentVariable {
    std::wstring_view name{};
    std::wstring_view value{};
};

std::optional<std::wstring> environment_variable(const wchar_t* name);
std::wstring environment_block_with(std::span<const EnvironmentVariable> variables);

}
