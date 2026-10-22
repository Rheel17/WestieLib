#include "platform/win32/Utf16.h"

#include <limits>
#include <stdexcept>

#include <windows.h>

#include "platform/win32/Win32Error.h"

namespace westie::platform::win32 {

std::wstring to_utf16(std::string_view utf8) {
    if (utf8.empty()) {
        return {};
    }
    if (utf8.size() > static_cast<std::size_t>(std::numeric_limits<int>::max())) {
        throw std::length_error("to_utf16: input is too long");
    }

    int input_length = static_cast<int>(utf8.size());
    int length = check(MultiByteToWideChar(CP_UTF8, 0, utf8.data(), input_length, nullptr, 0), "MultiByteToWideChar");

    std::wstring utf16;
    utf16.resize_and_overwrite(static_cast<std::size_t>(length), [&](wchar_t* buffer, std::size_t size) {
        return MultiByteToWideChar(CP_UTF8, 0, utf8.data(), input_length, buffer, static_cast<int>(size));
    });
    return utf16;
}

std::string to_utf8(std::wstring_view utf16) {
    if (utf16.empty()) {
        return {};
    }
    if (utf16.size() > static_cast<std::size_t>(std::numeric_limits<int>::max())) {
        throw std::length_error("to_utf8: input is too long");
    }

    int input_length = static_cast<int>(utf16.size());
    int length = check(WideCharToMultiByte(CP_UTF8, 0, utf16.data(), input_length, nullptr, 0, nullptr, nullptr), "WideCharToMultiByte");

    std::string utf8;
    utf8.resize_and_overwrite(static_cast<std::size_t>(length), [&](char* buffer, std::size_t size) {
        return WideCharToMultiByte(CP_UTF8, 0, utf16.data(), input_length, buffer, static_cast<int>(size), nullptr, nullptr);
    });
    return utf8;
}

}
