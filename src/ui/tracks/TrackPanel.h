#pragma once

#include <windows.h>

#include "ui/framework/Panel.h"

namespace westie::ui {

class TrackPanel final : public framework::Panel {
public:
    explicit TrackPanel(HWND parent);
};

}
