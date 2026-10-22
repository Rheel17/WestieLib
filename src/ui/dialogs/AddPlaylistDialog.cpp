#include "ui/dialogs/AddPlaylistDialog.h"

#include <string>
#include <utility>

#include "ResourceIds.h"
#include "platform/win32/Win32Error.h"
#include "ui/SourceLogo.h"
#include "ui/framework/Dpi.h"
#include "ui/framework/IconResource.h"
#include "ui/framework/SavedDc.h"

namespace westie::ui {

namespace {

constexpr int LogoDips = 16;
constexpr int LogoGapDips = 6;
constexpr UINT LineFormat = DT_SINGLELINE | DT_VCENTER | DT_NOPREFIX | DT_END_ELLIPSIS;

std::wstring dialog_item_text(HWND dialog, int id) {
    HWND item = GetDlgItem(dialog, id);
    std::wstring text;
    text.resize_and_overwrite(GetWindowTextLengthW(item), [item](wchar_t* buffer, std::size_t size) {
        return static_cast<std::size_t>(GetWindowTextW(item, buffer, static_cast<int>(size) + 1));
    });
    return text;
}

}

AddPlaylistDialog::AddPlaylistDialog(std::function<bool(const core::PlaylistLink&)> is_added) :
        ModalDialog(IDD_ADD_PLAYLIST), is_added_(std::move(is_added)) {}

std::optional<AddPlaylistRequest> AddPlaylistDialog::choose(HWND owner) {
    if (show_modal(owner) != IDOK) {
        return std::nullopt;
    }
    return request_;
}

INT_PTR AddPlaylistDialog::handle_message(UINT message, WPARAM wparam, LPARAM lparam) {
    switch (message) {
    case WM_INITDIALOG: on_init_dialog(); return TRUE;
    case WM_COMMAND:    on_command(wparam); return TRUE;
    case WM_DRAWITEM:   draw_link_line(*reinterpret_cast<const DRAWITEMSTRUCT*>(lparam)); return TRUE;
    default:            return ModalDialog::handle_message(message, wparam, lparam);
    }
}

void AddPlaylistDialog::on_init_dialog() {
    CheckDlgButton(hwnd(), IDC_SYNC_NOW, BST_CHECKED);
    check_link();
}

void AddPlaylistDialog::on_command(WPARAM wparam) {
    switch (LOWORD(wparam)) {
    case IDOK:              accept(); return;
    case IDCANCEL:          end(IDCANCEL); return;
    case IDC_PLAYLIST_LINK: if (HIWORD(wparam) == EN_CHANGE) { check_link(); } return;
    default:                return;
    }
}

void AddPlaylistDialog::check_link() {
    check_ = check_playlist_link(dialog_item_text(hwnd(), IDC_PLAYLIST_LINK), is_added_);
    // The line paints itself, so its window text is only for screen readers and the UI checks
    std::wstring text = link_check_text(check_);
    HWND line = GetDlgItem(hwnd(), IDC_PLAYLIST_KIND);
    platform::win32::check(SetWindowTextW(line, text.c_str()), "SetWindowTextW");
    platform::win32::check(InvalidateRect(line, nullptr, TRUE), "InvalidateRect");
    EnableWindow(GetDlgItem(hwnd(), IDOK), check_.state == LinkState::New);
}

void AddPlaylistDialog::draw_link_line(const DRAWITEMSTRUCT& item) const {
    framework::SavedDc saved(item.hDC);
    FillRect(item.hDC, &item.rcItem, GetSysColorBrush(COLOR_BTNFACE));

    RECT text = item.rcItem;
    if (check_.link) {
        framework::Dpi dpi = framework::Dpi::of(item.hwndItem);
        framework::IconResource logo(source_logo_icon(check_.link->type), dpi.scale(LogoDips));
        logo.draw(item.hDC, text.left, text.top + (text.bottom - text.top - logo.size()) / 2);
        text.left += logo.size() + dpi.scale(LogoGapDips);
    }

    if (auto font = reinterpret_cast<HFONT>(SendMessageW(item.hwndItem, WM_GETFONT, 0, 0))) {
        SelectObject(item.hDC, font);
    }
    SetBkMode(item.hDC, TRANSPARENT);
    SetTextColor(item.hDC, GetSysColor(check_.state == LinkState::NotSupported ? COLOR_GRAYTEXT : COLOR_WINDOWTEXT));
    std::wstring line = link_check_text(check_);
    DrawTextW(item.hDC, line.c_str(), static_cast<int>(line.size()), &text, LineFormat);
}

void AddPlaylistDialog::accept() {
    if (check_.state != LinkState::New) {
        return;
    }
    request_ = AddPlaylistRequest{.link = *check_.link, .sync_now = IsDlgButtonChecked(hwnd(), IDC_SYNC_NOW) == BST_CHECKED};
    end(IDOK);
}

}
