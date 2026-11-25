#include "ui/dialogs/DeleteFilesDialog.h"

#include <filesystem>
#include <format>
#include <string>

#include <windows.h>
#include <commctrl.h>

#include "platform/win32/Win32Error.h"
#include "ui/framework/Instance.h"

namespace westie::ui {

bool confirm_delete_files(HWND owner, const std::vector<std::wstring>& files) {
    bool one = files.size() == 1;
    std::wstring instruction =
            one ? std::format(L"Delete \"{}\"?", std::filesystem::path(files.front()).filename().wstring()) : std::format(L"Delete {} files?", files.size());
    TASKDIALOG_BUTTON remove{.nButtonID = IDOK, .pszButtonText = L"Delete"};

    TASKDIALOGCONFIG config{};
    config.cbSize = sizeof(config);
    config.hwndParent = owner;
    config.hInstance = framework::module_instance();
    config.dwFlags = TDF_ALLOW_DIALOG_CANCELLATION | TDF_POSITION_RELATIVE_TO_WINDOW;
    config.dwCommonButtons = TDCBF_CANCEL_BUTTON;
    config.pszWindowTitle = L"Delete file";
    config.pszMainInstruction = instruction.c_str();
    config.pszContent = one ? L"The file is deleted from the library folder, not moved to the Recycle Bin."
                            : L"The files are deleted from the library folder, not moved to the Recycle Bin.";
    config.cButtons = 1;
    config.pButtons = &remove;
    // Cancel is the default, so a stray Enter keeps the files
    config.nDefaultButton = IDCANCEL;

    int chosen = IDCANCEL;
    platform::win32::check_hresult(TaskDialogIndirect(&config, &chosen, nullptr, nullptr), "TaskDialogIndirect");
    return chosen == IDOK;
}

}
