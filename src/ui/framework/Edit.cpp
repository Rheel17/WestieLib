#include "ui/framework/Edit.h"

#include <algorithm>

#include <windows.h>
#include <commctrl.h>

#include "platform/win32/Win32Error.h"

namespace westie::ui::framework {

namespace {

constexpr int HeightDips = 23;
constexpr int TextHeightPaddingDips = 7;
constexpr int TextMarginDips = 4;

}

Edit::Edit(HWND parent) :
        Control({
            .class_name = WC_EDITW,
            .style = WS_CHILD | WS_VISIBLE | WS_TABSTOP | ES_AUTOHSCROLL,
            .ex_style = WS_EX_CLIENTEDGE,
            .parent = parent,
        }) {}

int Edit::height() const noexcept {
    return height_;
}

void Edit::apply_dpi(Dpi dpi, const MessageFont& font) {
    set_font(font.handle());
    // A new font resets the margins
    int margin = dpi.scale(TextMarginDips);
    SendMessageW(hwnd(), EM_SETMARGINS, EC_LEFTMARGIN | EC_RIGHTMARGIN, MAKELPARAM(margin, margin));
    height_ = std::max(dpi.scale(HeightDips), font.line_height() + dpi.scale(TextHeightPaddingDips));
}

std::wstring Edit::text() const {
    std::wstring text;
    text.resize_and_overwrite(GetWindowTextLengthW(hwnd()), [this](wchar_t* buffer, std::size_t size) {
        return static_cast<std::size_t>(GetWindowTextW(hwnd(), buffer, static_cast<int>(size) + 1));
    });
    return text;
}

void Edit::set_text(const std::wstring& text) {
    platform::win32::check(SetWindowTextW(hwnd(), text.c_str()), "SetWindowTextW");
}

}
