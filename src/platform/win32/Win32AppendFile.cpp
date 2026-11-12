#include "platform/win32/Win32AppendFile.h"

#include <limits>
#include <stdexcept>

#include <windows.h>

#include "platform/win32/Win32Error.h"

namespace westie::platform::win32 {

Win32AppendFile::Win32AppendFile(const std::filesystem::path& file) {
    // Shared for reading and writing, so an editor can open the file while the app writes it
    handle_ = adopt_file(CreateFileW(file.c_str(), FILE_APPEND_DATA, FILE_SHARE_READ | FILE_SHARE_WRITE, nullptr, OPEN_ALWAYS, FILE_ATTRIBUTE_NORMAL, nullptr));
    if (!handle_) {
        throw_last_error("CreateFileW");
    }
}

void Win32AppendFile::append(std::string_view text) {
    if (text.size() > std::numeric_limits<DWORD>::max()) {
        throw std::length_error("append: too much to write");
    }
    DWORD written = 0;
    check(WriteFile(handle_.get(), text.data(), static_cast<DWORD>(text.size()), &written, nullptr), "WriteFile");
}

}
