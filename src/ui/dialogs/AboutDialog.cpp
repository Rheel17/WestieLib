#include "ui/dialogs/AboutDialog.h"

#include <windows.h>
#include <commctrl.h>

#include "ResourceIds.h"
#include "Version.h"
#include "platform/win32/Win32Error.h"
#include "ui/framework/Instance.h"

namespace westie::ui {

void show_about_dialog(HWND owner) {
    TASKDIALOGCONFIG config{};
    config.cbSize = sizeof(config);
    config.hwndParent = owner;
    config.hInstance = framework::module_instance();
    config.dwFlags = TDF_ALLOW_DIALOG_CANCELLATION | TDF_POSITION_RELATIVE_TO_WINDOW;
    config.dwCommonButtons = TDCBF_OK_BUTTON;
    config.pszWindowTitle = L"About WestieLib";
    config.pszMainIcon = MAKEINTRESOURCEW(IDI_APP_ICON);
    config.pszMainInstruction = L"WestieLib";
    config.pszContent = L"Version " WESTIELIB_VERSION_STRING;
    platform::win32::check_hresult(TaskDialogIndirect(&config, nullptr, nullptr, nullptr), "TaskDialogIndirect");
}

}
