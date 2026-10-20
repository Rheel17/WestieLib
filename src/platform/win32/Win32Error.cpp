#include "platform/win32/Win32Error.h"

#include <string>
#include <system_error>

namespace westie::platform::win32 {

void throw_error(DWORD error, std::string_view operation) {
    throw std::system_error(static_cast<int>(error), std::system_category(), std::string(operation));
}

void throw_last_error(std::string_view operation) {
    DWORD error = GetLastError();
    if (error == ERROR_SUCCESS) {
        // Some APIs fail without setting an error code
        error = ERROR_UNIDENTIFIED_ERROR;
    }
    throw_error(error, operation);
}

void check_status(LSTATUS status, std::string_view operation) {
    if (status != ERROR_SUCCESS) {
        throw_error(static_cast<DWORD>(status), operation);
    }
}

}
