#pragma once

#include <windows.h>

namespace westie::ui::framework {

class AcceleratorTable;

class MessageLoop {
public:
    int run(HWND window, const AcceleratorTable& accelerators);
};

}
