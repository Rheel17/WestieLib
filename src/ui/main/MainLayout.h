#pragma once

#include "ui/framework/Dpi.h"
#include "ui/framework/Rect.h"

namespace westie::ui {

constexpr int PlaylistsMinDips = 250;
constexpr int TracksMinDips = 400;
constexpr int InspectorMinDips = 290;

struct PanelWidths {
    int playlists_dips = PlaylistsMinDips;
    int inspector_dips = InspectorMinDips;
};

struct BarHeights {
    int toolbar = 0;
    int status_bar = 0;
};

struct MainLayout {
    framework::Rect toolbar;
    framework::Rect playlists;
    framework::Rect playlists_splitter;
    framework::Rect tracks;
    framework::Rect inspector_splitter;
    framework::Rect inspector;
    framework::Rect status_bar;
};

MainLayout compute_main_layout(framework::Rect client, framework::Dpi dpi, const BarHeights& bars, const PanelWidths& widths);

PanelWidths move_playlists_splitter(const MainLayout& layout, framework::Dpi dpi, PanelWidths widths, int left);
PanelWidths move_inspector_splitter(const MainLayout& layout, framework::Dpi dpi, PanelWidths widths, int left);

}
