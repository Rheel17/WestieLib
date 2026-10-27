#include "ui/framework/Edit.h"

#include <algorithm>

#include <windows.h>
#include <commctrl.h>

#include "platform/win32/Win32Error.h"
#include "ui/framework/EditKeys.h"

namespace westie::ui::framework {

namespace {

constexpr WPARAM EnterChar = L'\r';
constexpr WPARAM EscapeChar = 0x1B;
constexpr int HeightDips = 23;
constexpr int TextHeightPaddingDips = 7;
constexpr int TextMarginDips = 4;
// A single-line edit leaves a pixel above its text when it has room for it
constexpr int TextInsetPixels = 1;

}

Edit::Edit(HWND parent, const EditOptions& options) :
        Control({
            .class_name = WC_EDITW,
            // An icon inside the field is a child window, which the edit's own drawing must leave alone
            .style = WS_CHILD | WS_VISIBLE | WS_TABSTOP | WS_CLIPCHILDREN | ES_AUTOHSCROLL,
            .ex_style = WS_EX_CLIENTEDGE,
            .parent = parent,
        }),
        leading_dips_(options.leading_dips),
        escape_clears_(options.escape_clears) {
    add_edit_keys(hwnd());
    if (options.cue_banner) {
        platform::win32::check(SendMessageW(hwnd(), EM_SETCUEBANNER, FALSE, reinterpret_cast<LPARAM>(options.cue_banner)), "EM_SETCUEBANNER");
    }
}

int Edit::height() const noexcept {
    return height_;
}

void Edit::apply_dpi(Dpi dpi, const MessageFont& font) {
    set_font(font.handle());
    // A new font resets the margins
    int margin = dpi.scale(TextMarginDips);
    SendMessageW(hwnd(), EM_SETMARGINS, EC_LEFTMARGIN | EC_RIGHTMARGIN, MAKELPARAM(margin + dpi.scale(leading_dips_), margin));
    height_ = std::max(dpi.scale(HeightDips), font.line_height() + dpi.scale(TextHeightPaddingDips));
    text_height_ = font.line_height();
    UINT flags = SWP_FRAMECHANGED | SWP_NOMOVE | SWP_NOSIZE | SWP_NOZORDER | SWP_NOACTIVATE;
    platform::win32::check(SetWindowPos(hwnd(), nullptr, 0, 0, 0, 0, flags), "SetWindowPos");
}

LRESULT Edit::handle_message(UINT message, WPARAM wparam, LPARAM lparam) {
    // A single-line edit beeps at the characters of Enter and Esc
    if (message == WM_CHAR && wparam == EnterChar) {
        return 0;
    }
    if (message == WM_CHAR && wparam == EscapeChar) {
        if (escape_clears_ && GetWindowTextLengthW(hwnd()) > 0) {
            select_all();
            SendMessageW(hwnd(), EM_REPLACESEL, FALSE, reinterpret_cast<LPARAM>(L""));
        }
        return 0;
    }

    LRESULT result = Control::handle_message(message, wparam, lparam);
    // A single-line edit puts its text near the top of its client area, so the client area starts lower to center it
    if (message == WM_NCCALCSIZE) {
        RECT& client = wparam ? reinterpret_cast<NCCALCSIZE_PARAMS*>(lparam)->rgrc[0] : *reinterpret_cast<RECT*>(lparam);
        int room = static_cast<int>(client.bottom - client.top) - text_height_;
        client.top += std::max(0, room / 2 - TextInsetPixels);
    }
    return result;
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

void Edit::select_all() {
    SendMessageW(hwnd(), EM_SETSEL, 0, -1);
}

}
