#include "platform/win32/Win32FileOps.h"

#include <limits>
#include <stdexcept>

#include <windows.h>

#include "platform/win32/UniqueHandle.h"
#include "platform/win32/Win32Error.h"

namespace westie::platform::win32 {

namespace {

std::filesystem::path temporary_for(const std::filesystem::path& file) {
    std::filesystem::path temporary = file;
    temporary += L".tmp";
    return temporary;
}

void write_flushed(const std::filesystem::path& file, std::string_view contents) {
    if (contents.size() > std::numeric_limits<DWORD>::max()) {
        throw std::length_error("replace_file: too much to write");
    }
    std::filesystem::create_directories(file.parent_path());

    UniqueFile handle = adopt_file(CreateFileW(file.c_str(), GENERIC_WRITE, 0, nullptr, CREATE_ALWAYS, FILE_ATTRIBUTE_NORMAL, nullptr));
    if (!handle) {
        throw_last_error("CreateFileW");
    }
    DWORD written = 0;
    check(WriteFile(handle.get(), contents.data(), static_cast<DWORD>(contents.size()), &written, nullptr), "WriteFile");
    check(FlushFileBuffers(handle.get()), "FlushFileBuffers");
}

}

bool Win32FileOps::is_file(const std::filesystem::path& file) const {
    DWORD attributes = GetFileAttributesW(file.c_str());
    if (attributes == INVALID_FILE_ATTRIBUTES) {
        DWORD error = GetLastError();
        if (error == ERROR_FILE_NOT_FOUND || error == ERROR_PATH_NOT_FOUND) {
            return false;
        }
        throw_error(error, "GetFileAttributesW");
    }
    return (attributes & FILE_ATTRIBUTE_DIRECTORY) == 0;
}

std::optional<std::string> Win32FileOps::read_file(const std::filesystem::path& file) const {
    UniqueFile handle = adopt_file(CreateFileW(file.c_str(), GENERIC_READ, FILE_SHARE_READ, nullptr, OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, nullptr));
    if (!handle) {
        DWORD error = GetLastError();
        if (error == ERROR_FILE_NOT_FOUND || error == ERROR_PATH_NOT_FOUND) {
            return std::nullopt;
        }
        throw_error(error, "CreateFileW");
    }

    LARGE_INTEGER size{};
    check(GetFileSizeEx(handle.get(), &size), "GetFileSizeEx");
    if (size.QuadPart > std::numeric_limits<DWORD>::max()) {
        throw std::length_error("read_file: the file is too big");
    }
    std::string contents(static_cast<std::size_t>(size.QuadPart), '\0');
    DWORD read = 0;
    check(ReadFile(handle.get(), contents.data(), static_cast<DWORD>(contents.size()), &read, nullptr), "ReadFile");
    contents.resize(read);
    return contents;
}

void Win32FileOps::replace_file(const std::filesystem::path& file, std::string_view contents) {
    // Renaming over the file replaces it in one step, so a crash leaves the old contents or the new, never half
    std::filesystem::path temporary = temporary_for(file);
    write_flushed(temporary, contents);
    check(MoveFileExW(temporary.c_str(), file.c_str(), MOVEFILE_REPLACE_EXISTING | MOVEFILE_WRITE_THROUGH), "MoveFileExW");
}

void Win32FileOps::replace_file_keeping_backup(const std::filesystem::path& file, const std::filesystem::path& backup, std::string_view contents) {
    std::filesystem::path temporary = temporary_for(file);
    write_flushed(temporary, contents);
    // ReplaceFileW needs a file to replace; the first save has no previous version to keep
    if (!is_file(file)) {
        check(MoveFileExW(temporary.c_str(), file.c_str(), MOVEFILE_REPLACE_EXISTING | MOVEFILE_WRITE_THROUGH), "MoveFileExW");
        return;
    }
    check(ReplaceFileW(file.c_str(), temporary.c_str(), backup.c_str(), REPLACEFILE_IGNORE_MERGE_ERRORS, nullptr, nullptr), "ReplaceFileW");
}

void Win32FileOps::create_folders(const std::filesystem::path& folder) {
    std::filesystem::create_directories(folder);
}

void Win32FileOps::remove_tree(const std::filesystem::path& path) {
    std::filesystem::remove_all(path);
}

void Win32FileOps::move_replacing(const std::filesystem::path& from, const std::filesystem::path& to) {
    check(MoveFileExW(from.c_str(), to.c_str(), MOVEFILE_REPLACE_EXISTING | MOVEFILE_WRITE_THROUGH), "MoveFileExW");
}

}
