#include "platform/win32/Pipe.h"

#include <array>
#include <cstddef>
#include <string_view>

#include "platform/win32/Win32Error.h"

namespace westie::platform::win32 {

namespace {

constexpr std::size_t ChunkBytes = 4096;

void hand_over_line(std::string_view line, const OutputLineHandler& on_line) {
    if (line.ends_with('\r')) {
        line.remove_suffix(1);
    }
    on_line(line);
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
    // Where the line that hasn't been handed over yet starts
    std::size_t line_start = 0;
    std::array<char, ChunkBytes> chunk;
    while (true) {
        DWORD read = 0;
        if (!ReadFile(pipe, chunk.data(), static_cast<DWORD>(chunk.size()), &read, nullptr)) {
            DWORD error = GetLastError();
            // Every writer closed its end
            if (error != ERROR_BROKEN_PIPE) {
                throw_error(error, "ReadFile");
            }
            if (on_line && line_start < data.size()) {
                hand_over_line(std::string_view(data).substr(line_start), on_line);
            }
            return data;
        }
        data.append(chunk.data(), read);
        if (!on_line) {
            continue;
        }
        for (std::size_t end = data.find('\n', line_start); end != std::string::npos; end = data.find('\n', line_start)) {
            hand_over_line(std::string_view(data).substr(line_start, end - line_start), on_line);
            line_start = end + 1;
        }
    }
}

}
