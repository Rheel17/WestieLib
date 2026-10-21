#include "ui/framework/Toolbar.h"

#include <algorithm>
#include <vector>

#include <windows.h>
#include <commctrl.h>

#include "platform/win32/Win32Error.h"

namespace westie::ui::framework {

Toolbar::Toolbar(HWND parent) :
        Control({
            .class_name = TOOLBARCLASSNAMEW,
            .style = WS_CHILD | WS_VISIBLE | TBSTYLE_FLAT | TBSTYLE_LIST | CCS_NORESIZE | CCS_NOPARENTALIGN | CCS_NODIVIDER,
            .parent = parent,
        }) {
    SendMessageW(hwnd(), TB_BUTTONSTRUCTSIZE, sizeof(TBBUTTON), 0);
    SendMessageW(hwnd(), TB_SETEXTENDEDSTYLE, 0, TBSTYLE_EX_DRAWDDARROWS | TBSTYLE_EX_DOUBLEBUFFER);
}

void Toolbar::add_buttons(std::span<const ToolbarButton> buttons) {
    std::vector<TBBUTTON> entries;
    for (const ToolbarButton& button : buttons) {
        TBBUTTON entry{};
        if (button.text) {
            entry.iBitmap = button.image;
            entry.idCommand = button.command;
            entry.fsState = TBSTATE_ENABLED;
            entry.fsStyle = static_cast<BYTE>(BTNS_AUTOSIZE | (button.dropdown ? BTNS_DROPDOWN : BTNS_BUTTON));
            entry.iString = reinterpret_cast<INT_PTR>(button.text);
        } else {
            entry.fsStyle = BTNS_SEP;
        }
        entries.push_back(entry);
    }
    platform::win32::check(SendMessageW(hwnd(), TB_ADDBUTTONSW, entries.size(), reinterpret_cast<LPARAM>(entries.data())), "TB_ADDBUTTONSW");
}

void Toolbar::set_image_lists(HIMAGELIST normal, HIMAGELIST disabled) {
    SendMessageW(hwnd(), TB_SETIMAGELIST, 0, reinterpret_cast<LPARAM>(normal));
    SendMessageW(hwnd(), TB_SETDISABLEDIMAGELIST, 0, reinterpret_cast<LPARAM>(disabled));
}

void Toolbar::apply_metrics(const ToolbarMetrics& metrics, HFONT font) {
    SendMessageW(hwnd(), TB_SETLISTGAP, metrics.icon_gap, 0);
    set_separator_widths(metrics.separator_width);

    // The toolbar measures its buttons only when it gets a font, and adds its theme's margins to their content.
    // Measure without vertical padding, then pad up to the height asked for. TB_SETBUTTONSIZE would leave a
    // default width behind, which clips the text of disabled buttons.
    set_padding(metrics.side_padding, 0, metrics.button_spacing);
    set_font(font);
    set_padding(metrics.side_padding, std::max(0, metrics.button_height - button_size().height), metrics.button_spacing);
    set_font(font);
}

void Toolbar::set_enabled(WORD command, bool enabled) {
    SendMessageW(hwnd(), TB_ENABLEBUTTON, command, MAKELPARAM(enabled, 0));
}

Size Toolbar::button_size() const {
    auto size = static_cast<DWORD>(SendMessageW(hwnd(), TB_GETBUTTONSIZE, 0, 0));
    return {LOWORD(size), HIWORD(size)};
}

std::vector<Rect> Toolbar::separator_rects() const {
    std::vector<Rect> rects;
    for (int index : separator_indexes()) {
        RECT rect;
        platform::win32::check(SendMessageW(hwnd(), TB_GETITEMRECT, index, reinterpret_cast<LPARAM>(&rect)), "TB_GETITEMRECT");
        rects.push_back(Rect::from_rect(rect));
    }
    return rects;
}

std::vector<int> Toolbar::separator_indexes() const {
    std::vector<int> indexes;
    auto count = static_cast<int>(SendMessageW(hwnd(), TB_BUTTONCOUNT, 0, 0));
    for (int index = 0; index < count; index++) {
        TBBUTTON button{};
        platform::win32::check(SendMessageW(hwnd(), TB_GETBUTTON, index, reinterpret_cast<LPARAM>(&button)), "TB_GETBUTTON");
        if (button.fsStyle & BTNS_SEP) {
            indexes.push_back(index);
        }
    }
    return indexes;
}

void Toolbar::set_separator_widths(int width) {
    for (int index : separator_indexes()) {
        TBBUTTONINFOW info{};
        info.cbSize = sizeof(info);
        info.dwMask = TBIF_BYINDEX | TBIF_SIZE;
        info.cx = static_cast<WORD>(width);
        platform::win32::check(SendMessageW(hwnd(), TB_SETBUTTONINFOW, index, reinterpret_cast<LPARAM>(&info)), "TB_SETBUTTONINFOW");
    }
}

void Toolbar::set_padding(int side, int vertical, int button_spacing) {
    TBMETRICS values{};
    values.cbSize = sizeof(values);
    values.dwMask = TBMF_PAD | TBMF_BUTTONSPACING;
    values.cxPad = 2 * side;
    values.cyPad = vertical;
    values.cxButtonSpacing = button_spacing;
    SendMessageW(hwnd(), TB_SETMETRICS, 0, reinterpret_cast<LPARAM>(&values));
}

}
