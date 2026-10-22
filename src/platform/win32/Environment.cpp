#include "platform/win32/Environment.h"

#include <algorithm>

#include <windows.h>

#include "platform/win32/UniqueHandle.h"
#include "platform/win32/Win32Error.h"

namespace westie::platform::win32 {

namespace {

bool same_name(std::wstring_view first, std::wstring_view second) {
    return CompareStringOrdinal(first.data(), static_cast<int>(first.size()), second.data(), static_cast<int>(second.size()), TRUE) == CSTR_EQUAL;
}

}

std::optional<std::wstring> environment_variable(const wchar_t* name) {
    // The variable can change between the calls, so ask until the buffer holds it
    std::wstring value;
    while (true) {
        SetLastError(ERROR_SUCCESS);
        DWORD length = GetEnvironmentVariableW(name, value.data(), static_cast<DWORD>(value.size() + 1));
        if (length == 0) {
            DWORD error = GetLastError();
            if (error == ERROR_ENVVAR_NOT_FOUND) {
                return std::nullopt;
            }
            if (error != ERROR_SUCCESS) {
                throw_error(error, "GetEnvironmentVariableW");
            }
        }
        if (length <= value.size()) {
            value.resize(length);
            return value;
        }
        // Without the variable fitting, the length counts the terminating null
        value.resize(length - 1);
    }
}

std::wstring environment_block_with(std::span<const EnvironmentVariable> variables) {
    UniqueEnvironmentStrings current(check(GetEnvironmentStringsW(), "GetEnvironmentStringsW"));
    std::wstring block;
    for (const wchar_t* entry = current.get(); *entry != L'\0';) {
        std::wstring_view variable(entry);
        entry += variable.size() + 1;
        // The hidden per-drive entries, such as =C:=C:\Music, start with their separator
        std::wstring_view name = variable.substr(0, variable.find(L'=', 1));
        if (std::ranges::none_of(variables, [name](const EnvironmentVariable& replaced) { return same_name(name, replaced.name); })) {
            block += variable;
            block += L'\0';
        }
    }
    for (const EnvironmentVariable& variable : variables) {
        block += variable.name;
        block += L'=';
        block += variable.value;
        block += L'\0';
    }
    block += L'\0';
    return block;
}

}
