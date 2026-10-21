#pragma once

#include <optional>

#include "ui/framework/Dpi.h"
#include "ui/framework/Rect.h"

namespace westie::ui {

constexpr int PlaylistsMinDips = 250;
constexpr int TracksMinDips = 400;
constexpr int InspectorMinDips = 290;
constexpr int LogDefaultDips = 168;
constexpr int LogMinDips = 100;
constexpr int PanelsMinHeightDips = 240;

struct PanelState {
    int playlists_dips = PlaylistsMinDips;
    int inspector_dips = InspectorMinDips;
    int log_dips = LogDefaultDips;
    bool log_visible = false;
};

struct BarHeights {
    int toolbar = 0;
    int infobar = 0;
    int status_bar = 0;
};

struct MainLayout {
    framework::Rect toolbar;
    std::optional<framework::Rect> infobar;
    framework::Rect playlists;
    framework::Rect playlists_splitter;
    framework::Rect tracks;
    framework::Rect inspector_splitter;
    framework::Rect inspector;
    std::optional<framework::Rect> log_splitter;
    std::optional<framework::Rect> log;
    framework::Rect status_bar;
};

MainLayout compute_main_layout(framework::Rect client, framework::Dpi dpi, const BarHeights& bars, const PanelState& panels);

PanelState move_playlists_splitter(const MainLayout& layout, framework::Dpi dpi, PanelState panels, int left);
PanelState move_inspector_splitter(const MainLayout& layout, framework::Dpi dpi, PanelState panels, int left);
PanelState move_log_splitter(const MainLayout& layout, framework::Dpi dpi, PanelState panels, int top);

}
