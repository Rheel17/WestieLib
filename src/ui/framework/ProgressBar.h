#pragma once

#include <windows.h>

#include "ui/framework/Control.h"

namespace westie::ui::framework {

class ProgressBar final : public Control {
public:
    explicit ProgressBar(HWND parent);

    void set_percent(int percent);
};

}
