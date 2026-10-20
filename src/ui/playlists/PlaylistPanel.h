#pragma once

#include <windows.h>

#include "ui/framework/Panel.h"

namespace westie::ui {

class PlaylistPanel final : public framework::Panel {
public:
    explicit PlaylistPanel(HWND parent);
};

}
