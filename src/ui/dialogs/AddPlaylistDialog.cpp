#include "ui/dialogs/AddPlaylistDialog.h"

#include <chrono>
#include <utility>

#include "ResourceIds.h"
#include "platform/win32/Win32Error.h"
#include "ui/dialogs/LinkLine.h"
#include "ui/dialogs/LinkLinePainter.h"
#include "ui/framework/EditKeys.h"

namespace westie::ui {

namespace {

constexpr UINT_PTR LookupTimer = 1;
// Long enough that a link typed by hand isn't looked up at every key
constexpr UINT LookupDelayMs = 300;
constexpr UINT_PTR SpinnerTimer = 2;
// About as often as a timer can tick
constexpr UINT SpinnerFrameMs = 16;
constexpr std::chrono::milliseconds SpinnerTurn{1000};

std::wstring dialog_item_text(HWND dialog, int id) {
    HWND item = GetDlgItem(dialog, id);
    std::wstring text;
    text.resize_and_overwrite(GetWindowTextLengthW(item), [item](wchar_t* buffer, std::size_t size) {
        return static_cast<std::size_t>(GetWindowTextW(item, buffer, static_cast<int>(size) + 1));
    });
    return text;
}

}

AddPlaylistDialog::AddPlaylistDialog(AddedPlaylistName added_name, core::PlaylistLookup& lookup, const ToolBoard& tools) :
        ModalDialog(IDD_ADD_PLAYLIST), added_name_(std::move(added_name)), lookup_(lookup, tools), spinner_(SpinnerTimer, SpinnerFrameMs) {}

std::optional<AddPlaylistRequest> AddPlaylistDialog::choose(HWND owner) {
    INT_PTR result = show_modal(owner);
    lookup_.stop();
    if (result != IDOK) {
        return std::nullopt;
    }
    return request_;
}

void AddPlaylistDialog::show_lookup(const core::PlaylistLookedUp& answer) {
    if (hwnd() && lookup_.apply(answer)) {
        show_line();
    }
}

void AddPlaylistDialog::show_tools() {
    if (hwnd() && lookup_.tools_changed()) {
        show_line();
    }
}

INT_PTR AddPlaylistDialog::handle_message(UINT message, WPARAM wparam, LPARAM lparam) {
    switch (message) {
    case WM_INITDIALOG: on_init_dialog(); return TRUE;
    case WM_COMMAND:    on_command(wparam); return TRUE;
    case WM_TIMER:      on_timer(wparam); return TRUE;
    case WM_DRAWITEM:
        paint_link_line(*reinterpret_cast<const DRAWITEMSTRUCT*>(lparam), link_line(check_, lookup_.status()), spinner_.progress(SpinnerTurn));
        return TRUE;
    default: return ModalDialog::handle_message(message, wparam, lparam);
    }
}

void AddPlaylistDialog::on_init_dialog() {
    framework::add_edit_keys(platform::win32::check(GetDlgItem(hwnd(), IDC_PLAYLIST_LINK), "GetDlgItem"));
    CheckDlgButton(hwnd(), IDC_SYNC_NOW, BST_CHECKED);
    check_link();
}

void AddPlaylistDialog::on_command(WPARAM wparam) {
    switch (LOWORD(wparam)) {
    case IDOK:     accept(); return;
    case IDCANCEL: end(IDCANCEL); return;
    case IDC_PLAYLIST_LINK:
        if (HIWORD(wparam) == EN_CHANGE) {
            check_link();
        }
        return;
    default: return;
    }
}

void AddPlaylistDialog::on_timer(WPARAM timer) {
    if (spinner_.is_tick(timer)) {
        // The line paints all of itself, so nothing needs erasing first
        platform::win32::check(InvalidateRect(GetDlgItem(hwnd(), IDC_PLAYLIST_KIND), nullptr, FALSE), "InvalidateRect");
        return;
    }
    if (timer != LookupTimer) {
        return;
    }
    KillTimer(hwnd(), LookupTimer);
    lookup_.start();
}

void AddPlaylistDialog::check_link() {
    check_ = check_playlist_link(dialog_item_text(hwnd(), IDC_PLAYLIST_LINK), added_name_);
    // Every change starts the delay again
    KillTimer(hwnd(), LookupTimer);
    if (lookup_.set_link(check_)) {
        platform::win32::check(SetTimer(hwnd(), LookupTimer, LookupDelayMs, nullptr), "SetTimer");
    }
    show_line();
}

void AddPlaylistDialog::show_line() {
    // The line paints itself, so its window text is only for screen readers and the UI checks
    LinkLine shown = link_line(check_, lookup_.status());
    std::wstring text = shown.accessible_text();
    HWND line = GetDlgItem(hwnd(), IDC_PLAYLIST_KIND);
    platform::win32::check(SetWindowTextW(line, text.c_str()), "SetWindowTextW");
    spinner_.run(hwnd(), shown.spinner);
    platform::win32::check(InvalidateRect(line, nullptr, FALSE), "InvalidateRect");

    HWND add = GetDlgItem(hwnd(), IDOK);
    std::wstring add_text = add_button_text(check_);
    platform::win32::check(SetWindowTextW(add, add_text.c_str()), "SetWindowTextW");
    EnableWindow(add, can_add());
}

bool AddPlaylistDialog::can_add() const noexcept {
    return check_.state == LinkState::New && lookup_.status().state == LookupState::Found;
}

void AddPlaylistDialog::accept() {
    // IDOK also comes from Enter, while the button is disabled
    if (!can_add()) {
        return;
    }
    request_ = AddPlaylistRequest{
        .link = *check_.link,
        .name = lookup_.status().name,
        .sync_now = IsDlgButtonChecked(hwnd(), IDC_SYNC_NOW) == BST_CHECKED,
    };
    end(IDOK);
}

}
