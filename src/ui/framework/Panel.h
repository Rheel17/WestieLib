#pragma once

#include <windows.h>

#include "ui/framework/Window.h"

namespace westie::ui::framework {

class Panel : public Window {
protected:
    Panel(HWND parent, const wchar_t* name);

private:
    static const WindowClass& window_class();
};

}
