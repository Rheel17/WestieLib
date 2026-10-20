#pragma once

#include <windows.h>

#include "ui/framework/Window.h"

namespace westie::ui::framework {

class Panel : public Window {
protected:
    explicit Panel(HWND parent);

private:
    static const WindowClass& window_class();
};

}
