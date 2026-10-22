#include "ui/framework/Panel.h"

#include "ui/framework/WindowClass.h"

namespace westie::ui::framework {

Panel::Panel(HWND parent, const wchar_t* name, DWORD extra_style) :
        Window(window_class(), {.title = name, .style = WS_CHILD | WS_VISIBLE | WS_CLIPCHILDREN | WS_CLIPSIBLINGS | extra_style, .parent = parent}) {}

const WindowClass& Panel::window_class() {
    static WindowClass window_class({.name = L"WestieLib.Panel", .background = GetSysColorBrush(COLOR_WINDOW)});
    return window_class;
}

}
