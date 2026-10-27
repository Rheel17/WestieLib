#pragma once

#include <map>
#include <optional>

#include "ui/main/MainLayout.h"
#include "ui/main/WindowPlacement.h"
#include "ui/tracks/TrackSort.h"

namespace westie::ui {

struct WindowState {
    Placement placement{};
    PanelState panels{};
    // Only the columns with a saved width
    std::map<TrackColumn, int> column_dips{};
};

std::optional<WindowState> load_window_state();
void save_window_state(const WindowState& state);

}
