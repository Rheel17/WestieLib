#pragma once

#include <windows.h>

#include "ui/framework/Panel.h"

namespace westie::ui {

class InspectorPanel final : public framework::Panel {
public:
    explicit InspectorPanel(HWND parent);
};

}
