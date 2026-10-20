#include "ui/FatalError.h"

#include <string>

#include <windows.h>

#include "platform/win32/Utf16.h"

namespace westie::ui {

namespace {

constexpr wchar_t Title[] = L"WestieLib";
constexpr wchar_t Intro[] = L"WestieLib ran into a problem and has to close.";

}

void show_fatal_error(std::string_view message) noexcept {
    try {
        std::wstring text = std::wstring(Intro) + L"\n\n" + platform::win32::to_utf16(message);
        MessageBoxW(nullptr, text.c_str(), Title, MB_OK | MB_ICONERROR);
    } catch (...) {
        MessageBoxW(nullptr, Intro, Title, MB_OK | MB_ICONERROR);
    }
}

}
