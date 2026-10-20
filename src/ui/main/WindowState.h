#pragma once

#include <optional>

#include "ui/framework/Rect.h"
#include "ui/main/MainLayout.h"

namespace westie::ui {

struct WindowState {
    framework::Rect normal_bounds;
    bool maximized = false;
    PanelWidths panel_widths;
};

std::optional<WindowState> load_window_state();
void save_window_state(const WindowState& state);

}
