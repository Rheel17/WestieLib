#pragma once

#include <windows.h>

#include "ui/framework/Window.h"

namespace westie::ui::framework {

class Panel : public Window {
protected:
    Panel(HWND parent, const wchar_t* name, DWORD extra_style = 0);

private:
    static const WindowClass& window_class();
};

}
