#include "platform/win32/Pipe.h"

#include <array>

#include "platform/win32/Win32Error.h"

namespace westie::platform::win32 {

namespace {

constexpr std::size_t ChunkBytes = 4096;

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

std::string read_all(HANDLE pipe) {
    std::string data;
    std::array<char, ChunkBytes> chunk;
    while (true) {
        DWORD read = 0;
        if (!ReadFile(pipe, chunk.data(), static_cast<DWORD>(chunk.size()), &read, nullptr)) {
            DWORD error = GetLastError();
            // Every writer closed its end
            if (error == ERROR_BROKEN_PIPE) {
                return data;
            }
            throw_error(error, "ReadFile");
        }
        data.append(chunk.data(), read);
    }
}

}
