#include "ui/framework/ModalDialog.h"

#include <algorithm>

#include "platform/win32/Win32Error.h"
#include "ui/framework/Instance.h"
#include "ui/framework/PendingException.h"
#include "ui/framework/Rect.h"

namespace westie::ui::framework {

ModalDialog::ModalDialog(int template_id) noexcept : template_id_(template_id) {}

HWND ModalDialog::hwnd() const noexcept {
    return hwnd_;
}

INT_PTR ModalDialog::show_modal(HWND owner) {
    INT_PTR result = DialogBoxParamW(module_instance(), MAKEINTRESOURCEW(template_id_), owner, dialog_proc, reinterpret_cast<LPARAM>(this));
    PendingException::rethrow_if_any();
    if (result == -1) {
        platform::win32::throw_last_error("DialogBoxParamW");
    }
    return result;
}

void ModalDialog::end(INT_PTR result) {
    platform::win32::check(EndDialog(hwnd_, result), "EndDialog");
}

INT_PTR ModalDialog::handle_message(UINT, WPARAM, LPARAM) {
    return FALSE;
}

INT_PTR CALLBACK ModalDialog::dialog_proc(HWND hwnd, UINT message, WPARAM wparam, LPARAM lparam) {
    if (message == WM_INITDIALOG) {
        auto* dialog = reinterpret_cast<ModalDialog*>(lparam);
        dialog->hwnd_ = hwnd;
        SetWindowLongPtrW(hwnd, DWLP_USER, lparam);
    }

    auto* dialog = reinterpret_cast<ModalDialog*>(GetWindowLongPtrW(hwnd, DWLP_USER));
    if (!dialog) {
        return FALSE;
    }

    INT_PTR result = FALSE;
    try {
        if (message == WM_INITDIALOG) {
            dialog->center_on_owner();
        }
        result = dialog->handle_message(message, wparam, lparam);
    } catch (...) {
        // The dialog's own message loop doesn't look for exceptions, so close it and let show_modal rethrow
        PendingException::capture();
        EndDialog(hwnd, IDCANCEL);
    }

    if (message == WM_NCDESTROY) {
        SetWindowLongPtrW(hwnd, DWLP_USER, 0);
        dialog->hwnd_ = nullptr;
    }
    return result;
}

void ModalDialog::center_on_owner() const {
    HWND owner = GetWindow(hwnd_, GW_OWNER);
    if (!owner) {
        return;
    }

    RECT owner_bounds;
    RECT dialog_bounds;
    platform::win32::check(GetWindowRect(owner, &owner_bounds), "GetWindowRect");
    platform::win32::check(GetWindowRect(hwnd_, &dialog_bounds), "GetWindowRect");
    MONITORINFO monitor{};
    monitor.cbSize = sizeof(monitor);
    platform::win32::check(GetMonitorInfoW(MonitorFromWindow(owner, MONITOR_DEFAULTTONEAREST), &monitor), "GetMonitorInfoW");

    Rect around = Rect::from_rect(owner_bounds);
    Rect dialog = Rect::from_rect(dialog_bounds);
    Rect work = Rect::from_rect(monitor.rcWork);
    // A dialog larger than the work area keeps its top left corner on screen
    int x = std::max(work.left, std::min(around.left + (around.width() - dialog.width()) / 2, work.right - dialog.width()));
    int y = std::max(work.top, std::min(around.top + (around.height() - dialog.height()) / 2, work.bottom - dialog.height()));
    platform::win32::check(SetWindowPos(hwnd_, nullptr, x, y, 0, 0, SWP_NOSIZE | SWP_NOZORDER | SWP_NOACTIVATE), "SetWindowPos");
}

}
