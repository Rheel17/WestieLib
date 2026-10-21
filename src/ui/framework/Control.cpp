#include "ui/framework/Control.h"

#include <windows.h>
#include <commctrl.h>

#include "platform/win32/Win32Error.h"
#include "ui/framework/Instance.h"
#include "ui/framework/PendingException.h"

namespace westie::ui::framework {

namespace {

constexpr UINT_PTR SubclassId = 1;

}

Control::Control(const CreateParams& params) {
    // A child's menu handle is its id
    auto id = reinterpret_cast<HMENU>(static_cast<UINT_PTR>(params.id));
    HWND hwnd = CreateWindowExW(params.ex_style, params.class_name, params.text, params.style, 0, 0, 0, 0, params.parent, id, module_instance(), nullptr);
    hwnd_ = platform::win32::check(hwnd, "CreateWindowExW");

    // The destructor doesn't run for a constructor that throws
    if (!SetWindowSubclass(hwnd_, subclass_proc, SubclassId, reinterpret_cast<DWORD_PTR>(this))) {
        DestroyWindow(hwnd_);
        platform::win32::throw_last_error("SetWindowSubclass");
    }
}

Control::~Control() {
    if (hwnd_) {
        RemoveWindowSubclass(hwnd_, subclass_proc, SubclassId);
        DestroyWindow(hwnd_);
    }
}

HWND Control::hwnd() const noexcept {
    return hwnd_;
}

void Control::set_font(HFONT font) {
    SendMessageW(hwnd_, WM_SETFONT, reinterpret_cast<WPARAM>(font), TRUE);
}

LRESULT Control::handle_message(UINT message, WPARAM wparam, LPARAM lparam) {
    return DefSubclassProc(hwnd_, message, wparam, lparam);
}

LRESULT CALLBACK Control::subclass_proc(HWND hwnd, UINT message, WPARAM wparam, LPARAM lparam, UINT_PTR id, DWORD_PTR data) {
    auto* control = reinterpret_cast<Control*>(data);
    // The parent can take the control down first; the destructor mustn't destroy a handle that may be reused
    if (message == WM_NCDESTROY) {
        RemoveWindowSubclass(hwnd, subclass_proc, id);
        control->hwnd_ = nullptr;
        return DefSubclassProc(hwnd, message, wparam, lparam);
    }

    try {
        return control->handle_message(message, wparam, lparam);
    } catch (...) {
        PendingException::capture();
        return 0;
    }
}

}
