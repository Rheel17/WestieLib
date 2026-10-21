#include "platform/win32/Environment.h"

#include <windows.h>

#include "platform/win32/Win32Error.h"

namespace westie::platform::win32 {

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

}
