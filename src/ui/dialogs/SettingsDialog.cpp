#include "ui/dialogs/SettingsDialog.h"

#include <chrono>
#include <filesystem>
#include <string>
#include <utility>

#include "ResourceIds.h"
#include "Version.h"
#include "platform/win32/Win32Error.h"
#include "ui/dialogs/FolderPicker.h"
#include "ui/tools/ToolStatusText.h"

namespace westie::ui {

SettingsDialog::SettingsDialog(core::Settings settings, const ToolBoard& tools, std::function<void()> on_check_for_updates) :
        ModalDialog(IDD_SETTINGS), settings_(std::move(settings)), tools_(tools), on_check_for_updates_(std::move(on_check_for_updates)) {}

std::optional<core::Settings> SettingsDialog::edit(HWND owner) {
    if (show_modal(owner) != IDOK) {
        return std::nullopt;
    }
    return settings_;
}

void SettingsDialog::show_tools(bool can_check_for_updates) {
    can_check_for_updates_ = can_check_for_updates;
    if (hwnd()) {
        show_tool_list();
    }
}

INT_PTR SettingsDialog::handle_message(UINT message, WPARAM wparam, LPARAM lparam) {
    switch (message) {
    case WM_INITDIALOG:     on_init_dialog(); return FALSE;
    case WM_CTLCOLORSTATIC: return on_static_color(reinterpret_cast<HDC>(wparam), reinterpret_cast<HWND>(lparam));
    case WM_COMMAND:        on_command(wparam); return TRUE;
    default:                return ModalDialog::handle_message(message, wparam, lparam);
    }
}

void SettingsDialog::on_init_dialog() {
    show_library_folder();
    tool_list_.emplace(GetDlgItem(hwnd(), IDC_TOOLS));
    CheckDlgButton(hwnd(), IDC_AUTO_UPDATE_TOOLS, settings_.auto_update_tools ? BST_CHECKED : BST_UNCHECKED);
    show_tool_list();
    platform::win32::check(SetDlgItemTextW(hwnd(), IDC_VERSION, L"WestieLib " WESTIELIB_VERSION_STRING), "SetDlgItemTextW");
    // The folder edit is the first tab stop, and focusing it would open the dialog with the whole path selected
    SendMessageW(hwnd(), WM_NEXTDLGCTL, reinterpret_cast<WPARAM>(GetDlgItem(hwnd(), IDC_CHANGE_LIBRARY_FOLDER)), TRUE);
}

INT_PTR SettingsDialog::on_static_color(HDC dc, HWND control) const {
    if (control != GetDlgItem(hwnd(), IDC_VERSION) && control != GetDlgItem(hwnd(), IDC_LAST_CHECKED)) {
        return FALSE;
    }
    SetTextColor(dc, GetSysColor(COLOR_GRAYTEXT));
    SetBkColor(dc, GetSysColor(COLOR_BTNFACE));
    return reinterpret_cast<INT_PTR>(GetSysColorBrush(COLOR_BTNFACE));
}

void SettingsDialog::on_command(WPARAM wparam) {
    switch (LOWORD(wparam)) {
    case IDOK:                      accept(); return;
    case IDCANCEL:                  end(IDCANCEL); return;
    case IDC_CHANGE_LIBRARY_FOLDER: change_library_folder(); return;
    case IDC_CHECK_FOR_UPDATES:     on_check_for_updates_(); return;
    default:                        return;
    }
}

void SettingsDialog::change_library_folder() {
    if (std::optional<std::filesystem::path> folder = pick_folder(hwnd(), L"Library folder", settings_.library_path)) {
        settings_.library_path = std::move(*folder);
        show_library_folder();
    }
}

void SettingsDialog::show_library_folder() const {
    platform::win32::check(SetDlgItemTextW(hwnd(), IDC_LIBRARY_FOLDER, settings_.library_path.c_str()), "SetDlgItemTextW");
}

void SettingsDialog::show_tool_list() {
    tool_list_->show(tools_);
    std::wstring checked = last_checked_text(tools_, std::chrono::system_clock::now());
    platform::win32::check(SetDlgItemTextW(hwnd(), IDC_LAST_CHECKED, checked.c_str()), "SetDlgItemTextW");

    // A disabled button can't keep the focus, and without it the keyboard can't reach the dialog
    HWND check_button = GetDlgItem(hwnd(), IDC_CHECK_FOR_UPDATES);
    if (!can_check_for_updates_ && GetFocus() == check_button) {
        SendMessageW(hwnd(), WM_NEXTDLGCTL, 0, FALSE);
    }
    EnableWindow(check_button, can_check_for_updates_);
}

void SettingsDialog::accept() {
    settings_.auto_update_tools = IsDlgButtonChecked(hwnd(), IDC_AUTO_UPDATE_TOOLS) == BST_CHECKED;
    end(IDOK);
}

}
