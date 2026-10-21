#include "ui/dialogs/SettingsDialog.h"

#include <filesystem>
#include <utility>

#include "ResourceIds.h"
#include "Version.h"
#include "platform/win32/Win32Error.h"
#include "ui/dialogs/FolderPicker.h"

namespace westie::ui {

SettingsDialog::SettingsDialog(core::Settings settings) : ModalDialog(IDD_SETTINGS), settings_(std::move(settings)) {}

std::optional<core::Settings> SettingsDialog::edit(HWND owner) {
    if (show_modal(owner) != IDOK) {
        return std::nullopt;
    }
    return settings_;
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
    platform::win32::check(SetDlgItemTextW(hwnd(), IDC_VERSION, L"WestieLib " WESTIELIB_VERSION_STRING), "SetDlgItemTextW");
    // The folder edit is the first tab stop, and focusing it would open the dialog with the whole path selected
    SendMessageW(hwnd(), WM_NEXTDLGCTL, reinterpret_cast<WPARAM>(GetDlgItem(hwnd(), IDC_CHANGE_LIBRARY_FOLDER)), TRUE);
}

INT_PTR SettingsDialog::on_static_color(HDC dc, HWND control) const {
    if (control != GetDlgItem(hwnd(), IDC_VERSION)) {
        return FALSE;
    }
    SetTextColor(dc, GetSysColor(COLOR_GRAYTEXT));
    SetBkColor(dc, GetSysColor(COLOR_BTNFACE));
    return reinterpret_cast<INT_PTR>(GetSysColorBrush(COLOR_BTNFACE));
}

void SettingsDialog::on_command(WPARAM wparam) {
    switch (LOWORD(wparam)) {
    case IDOK:
    case IDCANCEL:                  end(LOWORD(wparam)); return;
    case IDC_CHANGE_LIBRARY_FOLDER: change_library_folder(); return;
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

}
