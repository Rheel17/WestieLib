#include "platform/win32/KnownFolder.h"

#include <windows.h>
#include <shlobj.h>

#include "platform/win32/UniqueHandle.h"
#include "platform/win32/Win32Error.h"

namespace westie::platform::win32 {

std::filesystem::path known_folder(const KNOWNFOLDERID& id) {
    PWSTR path = nullptr;
    HRESULT result = SHGetKnownFolderPath(id, KF_FLAG_DEFAULT, nullptr, &path);
    // The string is ours to free, even when the call fails
    UniqueCoTaskString owned(path);
    check_hresult(result, "SHGetKnownFolderPath");
    return owned.get();
}

}
