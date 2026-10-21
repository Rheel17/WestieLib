#pragma once

#include <windows.h>

#include "ui/framework/Panel.h"

namespace westie::ui {

class LogPanel final : public framework::Panel {
public:
    explicit LogPanel(HWND parent);
};

}
