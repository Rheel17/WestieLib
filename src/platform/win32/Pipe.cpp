#include "platform/win32/Pipe.h"

#include <array>
#include <cstddef>
#include <string_view>

#include "platform/win32/Utf16.h"
#include "platform/win32/Win32Error.h"

namespace westie::platform::win32 {

namespace {

constexpr std::size_t ChunkBytes = 4096;

void take_line(std::string_view line, std::string& data, const OutputLineHandler& on_line) {
    std::size_t start = data.size();
    // A frozen Python program such as yt-dlp.exe ignores PYTHONIOENCODING, and writes some lines in the ANSI code page
    if (is_utf8(line)) {
        data += line;
    } else {
        data += ansi_to_utf8(line);
    }
    if (!on_line) {
        return;
    }
    std::string_view taken = std::string_view(data).substr(start);
    if (taken.ends_with('\n')) {
        taken.remove_suffix(1);
    }
    if (taken.ends_with('\r')) {
        taken.remove_suffix(1);
    }
    on_line(taken);
}

}

Pipe create_output_pipe() {
    SECURITY_ATTRIBUTES attributes{.nLength = sizeof(SECURITY_ATTRIBUTES), .lpSecurityDescriptor = nullptr, .bInheritHandle = TRUE};
    HANDLE read = nullptr;
    HANDLE write = nullptr;
    check(CreatePipe(&read, &write, &attributes, 0), "CreatePipe");
    Pipe pipe{.read = UniqueFile(read), .write = UniqueFile(write)};
    // Only the child writes into it
    check(SetHandleInformation(read, HANDLE_FLAG_INHERIT, 0), "SetHandleInformation");
    return pipe;
}

std::string read_all(HANDLE pipe, const OutputLineHandler& on_line) {
    std::string data;
    // The bytes of the line that hasn't ended yet
    std::string pending;
    std::array<char, ChunkBytes> chunk;
    while (true) {
        DWORD read = 0;
        if (!ReadFile(pipe, chunk.data(), static_cast<DWORD>(chunk.size()), &read, nullptr)) {
            DWORD error = GetLastError();
            // Every writer closed its end
            if (error != ERROR_BROKEN_PIPE) {
                throw_error(error, "ReadFile");
            }
            if (!pending.empty()) {
                take_line(pending, data, on_line);
            }
            return data;
        }
        std::size_t searched = pending.size();
        pending.append(chunk.data(), read);
        std::size_t line_start = 0;
        for (std::size_t end = pending.find('\n', searched); end != std::string::npos; end = pending.find('\n', line_start)) {
            take_line(std::string_view(pending).substr(line_start, end + 1 - line_start), data, on_line);
            line_start = end + 1;
        }
        pending.erase(0, line_start);
    }
}

}
