#include "platform/win32/ShellOpen.h"

#include <shellapi.h>

namespace westie::platform::win32 {

bool open_with_shell(HWND owner, const std::wstring& target) noexcept {
    SHELLEXECUTEINFOW info{};
    info.cbSize = sizeof(info);
    info.fMask = SEE_MASK_NOASYNC;
    info.hwnd = owner;
    info.lpVerb = L"open";
    info.lpFile = target.c_str();
    info.nShow = SW_SHOWNORMAL;
    // Without SEE_MASK_FLAG_NO_UI the shell tells the user itself when it can't open the target, so a failure needs
    // no more than the result
    return ShellExecuteExW(&info) != FALSE;
}

}
