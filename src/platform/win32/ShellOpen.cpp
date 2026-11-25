#include "platform/win32/ShellOpen.h"

#include <memory>

#include <shellapi.h>
#include <shlobj.h>

namespace westie::platform::win32 {

namespace {

struct IdListFreer {
    void operator()(ITEMIDLIST* list) const noexcept { ILFree(list); }
};

}

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

bool show_in_folder(const std::filesystem::path& file) noexcept {
    std::unique_ptr<ITEMIDLIST, IdListFreer> item(ILCreateFromPathW(file.c_str()));
    // With no items of its own to select, the list names the file itself
    return item && SUCCEEDED(SHOpenFolderAndSelectItems(item.get(), 0, nullptr, 0));
}

}
