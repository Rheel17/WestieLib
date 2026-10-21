#pragma once

#include <optional>

#include "ui/main/MainLayout.h"
#include "ui/main/WindowPlacement.h"

namespace westie::ui {

struct WindowState {
    Placement placement{};
    PanelState panels{};
};

std::optional<WindowState> load_window_state();
void save_window_state(const WindowState& state);

}
