#include "ui/framework/ComboBox.h"

#include <algorithm>

#include <windows.h>
#include <commctrl.h>

#include "platform/win32/Win32Error.h"

namespace westie::ui::framework {

namespace {

constexpr int HeightDips = 23;
constexpr int TextHeightPaddingDips = 7;

}

ComboBox::ComboBox(HWND parent) :
        Control({
            .class_name = WC_COMBOBOXW,
            .style = WS_CHILD | WS_VISIBLE | WS_TABSTOP | WS_VSCROLL | CBS_DROPDOWNLIST,
            .parent = parent,
        }) {}

void ComboBox::add_items(std::span<const wchar_t* const> items) {
    for (const wchar_t* item : items) {
        LRESULT index = SendMessageW(hwnd(), CB_ADDSTRING, 0, reinterpret_cast<LPARAM>(item));
        if (index == CB_ERR || index == CB_ERRSPACE) {
            platform::win32::throw_last_error("CB_ADDSTRING");
        }
    }
}

int ComboBox::selected() const {
    return static_cast<int>(SendMessageW(hwnd(), CB_GETCURSEL, 0, 0));
}

void ComboBox::select(int index) {
    SendMessageW(hwnd(), CB_SETCURSEL, index, 0);
}

int ComboBox::height() const noexcept {
    return height_;
}

void ComboBox::apply_dpi(Dpi dpi, const MessageFont& font) {
    set_font(font.handle());

    // The closed box is as tall as its selection field and its frame. A drop-down list sizes itself to that, whatever
    // height it's placed with, so set the field to give the height of an edit.
    RECT window;
    platform::win32::check(GetWindowRect(hwnd(), &window), "GetWindowRect");
    auto field = static_cast<int>(SendMessageW(hwnd(), CB_GETITEMHEIGHT, static_cast<WPARAM>(-1), 0));
    int frame = static_cast<int>(window.bottom - window.top) - field;
    int height = std::max(dpi.scale(HeightDips), font.line_height() + dpi.scale(TextHeightPaddingDips));
    if (SendMessageW(hwnd(), CB_SETITEMHEIGHT, static_cast<WPARAM>(-1), height - frame) == CB_ERR) {
        platform::win32::throw_last_error("CB_SETITEMHEIGHT");
    }
    height_ = height;
}

}
