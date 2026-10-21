#pragma once

#include <string_view>

#include <windows.h>

namespace westie::platform::win32 {

[[noreturn]] void throw_error(DWORD error, std::string_view operation);
[[noreturn]] void throw_last_error(std::string_view operation);

void check_status(LSTATUS status, std::string_view operation);
void check_hresult(HRESULT result, std::string_view operation);

template<typename T>
T check(T result, std::string_view operation) {
    if (!result) {
        throw_last_error(operation);
    }
    return result;
}

}
