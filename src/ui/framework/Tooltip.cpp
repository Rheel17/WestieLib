#include "ui/framework/Tooltip.h"

#include <climits>

#include <windows.h>
#include <commctrl.h>

#include "platform/win32/Win32Error.h"

namespace westie::ui::framework {

Tooltip::Tooltip(HWND owner) :
        Control({
            .class_name = TOOLTIPS_CLASSW,
            .style = WS_POPUP | TTS_ALWAYSTIP | TTS_NOPREFIX,
            .ex_style = WS_EX_TOPMOST,
            .parent = owner,
        }),
        owner_(owner) {
    // Without a maximum width a tooltip ignores line breaks
    SendMessageW(hwnd(), TTM_SETMAXTIPWIDTH, 0, SHRT_MAX);
}

void Tooltip::set_tool(UINT_PTR id, const Rect& bounds, const std::wstring& text) {
    TOOLINFOW tool{};
    tool.cbSize = sizeof(tool);
    tool.uFlags = TTF_SUBCLASS;
    tool.hwnd = owner_;
    tool.uId = id;
    tool.rect = bounds.to_rect();
    // The control copies the text
    tool.lpszText = const_cast<wchar_t*>(text.c_str());

    if (tools_.insert(id).second) {
        platform::win32::check(SendMessageW(hwnd(), TTM_ADDTOOLW, 0, reinterpret_cast<LPARAM>(&tool)), "TTM_ADDTOOLW");
    } else {
        SendMessageW(hwnd(), TTM_SETTOOLINFOW, 0, reinterpret_cast<LPARAM>(&tool));
    }
}

void Tooltip::remove_tool(UINT_PTR id) {
    if (tools_.erase(id) == 0) {
        return;
    }
    TOOLINFOW tool{};
    tool.cbSize = sizeof(tool);
    tool.hwnd = owner_;
    tool.uId = id;
    SendMessageW(hwnd(), TTM_DELTOOLW, 0, reinterpret_cast<LPARAM>(&tool));
}

}
