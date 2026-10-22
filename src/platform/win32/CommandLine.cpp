#include "platform/win32/CommandLine.h"

#include <string_view>

#include "platform/win32/Utf16.h"

namespace westie::platform::win32 {

namespace {

constexpr std::wstring_view NeedsQuotes = L" \t\n\v\"";

// The C runtime's rules: backslashes are literal, except a run of them before a quote, which they escape in pairs
void append_argument(std::wstring& line, std::wstring_view argument) {
    if (!argument.empty() && argument.find_first_of(NeedsQuotes) == std::wstring_view::npos) {
        line += argument;
        return;
    }

    line += L'"';
    std::size_t backslashes = 0;
    for (wchar_t c : argument) {
        if (c == L'\\') {
            ++backslashes;
            continue;
        }
        line.append(c == L'"' ? 2 * backslashes + 1 : backslashes, L'\\');
        line += c;
        backslashes = 0;
    }
    // The closing quote follows them
    line.append(2 * backslashes, L'\\');
    line += L'"';
}

}

std::wstring command_line(const std::filesystem::path& program, std::span<const std::string> arguments) {
    // The program's name takes everything between its quotes, which a path can't contain
    std::wstring line = L"\"" + program.wstring() + L"\"";
    for (const std::string& argument : arguments) {
        line += L' ';
        append_argument(line, to_utf16(argument));
    }
    return line;
}

}
