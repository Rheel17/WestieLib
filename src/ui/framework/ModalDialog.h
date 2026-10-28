#pragma once

#include <string>

#include <windows.h>

namespace westie::ui::framework {

class ModalDialog {
public:
    virtual ~ModalDialog() = default;

    ModalDialog(const ModalDialog&) = delete;
    ModalDialog& operator=(const ModalDialog&) = delete;

protected:
    explicit ModalDialog(int template_id) noexcept;

    HWND hwnd() const noexcept;
    INT_PTR show_modal(HWND owner);
    void end(INT_PTR result);
    std::wstring item_text(int id) const;
    // For a message whose result isn't the dialog procedure's, such as WM_NOTIFY's
    INT_PTR message_result(LRESULT result) const;

    virtual INT_PTR handle_message(UINT message, WPARAM wparam, LPARAM lparam);

private:
    static INT_PTR CALLBACK dialog_proc(HWND hwnd, UINT message, WPARAM wparam, LPARAM lparam);

    void center_on_owner() const;

    int template_id_ = 0;
    HWND hwnd_ = nullptr;
};

}
