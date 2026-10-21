#include "platform/win32/PathProbe.h"

#include <string>

#include <windows.h>

namespace westie::platform::win32 {

PathKind probe_path(const std::filesystem::path& path) {
    // A share, a mapped network drive, or a drive of an unknown kind can take long to answer, or never
    std::wstring root = path.root_path().native();
    if (root.starts_with(L"\\\\")) {
        return PathKind::Unknown;
    }
    switch (GetDriveTypeW(root.c_str())) {
    case DRIVE_NO_ROOT_DIR: return PathKind::NoSuchDrive;
    case DRIVE_REMOTE:
    case DRIVE_UNKNOWN:     return PathKind::Unknown;
    default:                break;
    }

    DWORD attributes = GetFileAttributesW(path.c_str());
    if (attributes == INVALID_FILE_ATTRIBUTES) {
        DWORD error = GetLastError();
        return error == ERROR_FILE_NOT_FOUND || error == ERROR_PATH_NOT_FOUND ? PathKind::Nothing : PathKind::Unknown;
    }
    return attributes & FILE_ATTRIBUTE_DIRECTORY ? PathKind::Folder : PathKind::File;
}

}
