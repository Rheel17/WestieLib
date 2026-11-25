#include "platform/win32/Utf16.h"

#include <limits>
#include <stdexcept>

#include <windows.h>

#include "platform/win32/Win32Error.h"

namespace westie::platform::win32 {

namespace {

int checked_length(std::size_t size, const char* function) {
    if (size > static_cast<std::size_t>(std::numeric_limits<int>::max())) {
        throw std::length_error(std::string(function) + ": input is too long");
    }
    return static_cast<int>(size);
}

std::wstring decode(UINT code_page, std::string_view text) {
    if (text.empty()) {
        return {};
    }
    int input_length = checked_length(text.size(), "MultiByteToWideChar");
    int length = check(MultiByteToWideChar(code_page, 0, text.data(), input_length, nullptr, 0), "MultiByteToWideChar");

    std::wstring utf16;
    utf16.resize_and_overwrite(static_cast<std::size_t>(length), [&](wchar_t* buffer, std::size_t size) {
        return MultiByteToWideChar(code_page, 0, text.data(), input_length, buffer, static_cast<int>(size));
    });
    return utf16;
}

}

std::wstring to_utf16(std::string_view utf8) {
    return decode(CP_UTF8, utf8);
}

std::string to_utf8(std::wstring_view utf16) {
    if (utf16.empty()) {
        return {};
    }
    int input_length = checked_length(utf16.size(), "WideCharToMultiByte");
    int length = check(WideCharToMultiByte(CP_UTF8, 0, utf16.data(), input_length, nullptr, 0, nullptr, nullptr), "WideCharToMultiByte");

    std::string utf8;
    utf8.resize_and_overwrite(static_cast<std::size_t>(length), [&](char* buffer, std::size_t size) {
        return WideCharToMultiByte(CP_UTF8, 0, utf16.data(), input_length, buffer, static_cast<int>(size), nullptr, nullptr);
    });
    return utf8;
}

bool is_utf8(std::string_view text) {
    return text.empty()
        || MultiByteToWideChar(CP_UTF8, MB_ERR_INVALID_CHARS, text.data(), checked_length(text.size(), "MultiByteToWideChar"), nullptr, 0) != 0;
}

std::string ansi_to_utf8(std::string_view ansi) {
    return to_utf8(decode(CP_ACP, ansi));
}

}
