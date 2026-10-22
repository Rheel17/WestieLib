#include "ui/dialogs/RemovePlaylistDialog.h"

#include <format>
#include <string>

#include <windows.h>
#include <commctrl.h>

#include "platform/win32/Win32Error.h"
#include "ui/framework/Instance.h"

namespace westie::ui {

bool confirm_remove_playlist(HWND owner, std::wstring_view name) {
    std::wstring instruction = std::format(L"Remove \"{}\"?", name);
    TASKDIALOG_BUTTON remove{.nButtonID = IDOK, .pszButtonText = L"Remove"};

    TASKDIALOGCONFIG config{};
    config.cbSize = sizeof(config);
    config.hwndParent = owner;
    config.hInstance = framework::module_instance();
    config.dwFlags = TDF_ALLOW_DIALOG_CANCELLATION | TDF_POSITION_RELATIVE_TO_WINDOW;
    config.dwCommonButtons = TDCBF_CANCEL_BUTTON;
    config.pszWindowTitle = L"Remove playlist";
    config.pszMainInstruction = instruction.c_str();
    config.pszContent = L"Synced tracks will not be deleted from your library.";
    config.cButtons = 1;
    config.pButtons = &remove;
    // Cancel is the default, so a stray Enter keeps the playlist
    config.nDefaultButton = IDCANCEL;

    int chosen = IDCANCEL;
    platform::win32::check_hresult(TaskDialogIndirect(&config, &chosen, nullptr, nullptr), "TaskDialogIndirect");
    return chosen == IDOK;
}

}
