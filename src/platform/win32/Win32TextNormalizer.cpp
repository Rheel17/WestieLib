#include "platform/win32/Win32TextNormalizer.h"

#include <cstddef>
#include <vector>

#include <windows.h>

#include "platform/win32/Utf16.h"
#include "platform/win32/Win32Error.h"

namespace westie::platform::win32 {

namespace {

// NormalizeString's first answer is an estimate, which can fall short
constexpr int NormalizeAttempts = 3;

std::wstring decomposed(const std::wstring& text) {
    int size = NormalizeString(NormalizationKD, text.c_str(), static_cast<int>(text.size()), nullptr, 0);
    for (int attempt = 0; attempt < NormalizeAttempts && size > 0; ++attempt) {
        std::wstring result(static_cast<std::size_t>(size), L'\0');
        int length = NormalizeString(NormalizationKD, text.c_str(), static_cast<int>(text.size()), result.data(), size);
        if (length > 0) {
            result.resize(static_cast<std::size_t>(length));
            return result;
        }
        if (GetLastError() != ERROR_INSUFFICIENT_BUFFER) {
            break;
        }
        size = -length;
    }
    throw_last_error("NormalizeString");
}

std::wstring without_marks(const std::wstring& text) {
    std::vector<WORD> types(text.size());
    check(GetStringTypeW(CT_CTYPE3, text.c_str(), static_cast<int>(text.size()), types.data()), "GetStringTypeW");
    std::wstring result;
    for (std::size_t i = 0; i < text.size(); ++i) {
        if ((types[i] & C3_NONSPACING) == 0) {
            result += text[i];
        }
    }
    return result;
}

std::wstring lowercase(const std::wstring& text) {
    std::wstring result(text.size(), L'\0');
    int length = check(
            LCMapStringEx(
                    LOCALE_NAME_INVARIANT, LCMAP_LOWERCASE, text.c_str(), static_cast<int>(text.size()), result.data(), static_cast<int>(result.size()), nullptr,
                    nullptr, 0
            ),
            "LCMapStringEx"
    );
    result.resize(static_cast<std::size_t>(length));
    return result;
}

}

std::string Win32TextNormalizer::normalize(std::string_view text) const {
    std::wstring wide = to_utf16(text);
    if (wide.empty()) {
        return {};
    }
    wide = without_marks(decomposed(wide));
    return wide.empty() ? std::string() : to_utf8(lowercase(wide));
}

}
