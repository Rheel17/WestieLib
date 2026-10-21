#include "ui/main/MainLayout.h"

#include <algorithm>

namespace westie::ui {

namespace {

constexpr int SplitterThicknessDips = 5;

}

MainLayout compute_main_layout(framework::Rect client, framework::Dpi dpi, const BarHeights& bars, const PanelState& panels) {
    int splitter = dpi.scale(SplitterThicknessDips);

    MainLayout layout;
    layout.toolbar = client.take_top(bars.toolbar);
    if (bars.infobar > 0) {
        layout.infobar = client.take_top(bars.infobar);
    }
    layout.status_bar = client.take_bottom(bars.status_bar);

    if (panels.log_visible) {
        int log_min = dpi.scale(LogMinDips);
        int room = client.height() - splitter - dpi.scale(PanelsMinHeightDips);
        layout.log = client.take_bottom(std::max(log_min, std::min(dpi.scale(panels.log_dips), room)));
        layout.log_splitter = client.take_bottom(splitter);
    }

    int playlists_min = dpi.scale(PlaylistsMinDips);
    int inspector_min = dpi.scale(InspectorMinDips);
    int playlists = std::max(dpi.scale(panels.playlists_dips), playlists_min);
    int inspector = std::max(dpi.scale(panels.inspector_dips), inspector_min);

    int overflow = playlists + inspector + 2 * splitter + dpi.scale(TracksMinDips) - client.width();
    int inspector_cut = std::clamp(overflow, 0, inspector - inspector_min);
    inspector -= inspector_cut;
    playlists -= std::clamp(overflow - inspector_cut, 0, playlists - playlists_min);

    layout.playlists = client.take_left(playlists);
    layout.playlists_splitter = client.take_left(splitter);
    layout.inspector = client.take_right(inspector);
    layout.inspector_splitter = client.take_right(splitter);
    layout.tracks = client;
    return layout;
}

PanelState move_playlists_splitter(const MainLayout& layout, framework::Dpi dpi, PanelState panels, int left) {
    int narrowest = dpi.scale(PlaylistsMinDips);
    int widest = layout.tracks.right - dpi.scale(TracksMinDips) - layout.playlists_splitter.width() - layout.playlists.left;
    panels.playlists_dips = dpi.unscale(std::clamp(left - layout.playlists.left, narrowest, std::max(narrowest, widest)));
    return panels;
}

PanelState move_inspector_splitter(const MainLayout& layout, framework::Dpi dpi, PanelState panels, int left) {
    int splitter = layout.inspector_splitter.width();
    int narrowest = dpi.scale(InspectorMinDips);
    int widest = layout.inspector.right - splitter - layout.playlists_splitter.right - dpi.scale(TracksMinDips);
    panels.inspector_dips = dpi.unscale(std::clamp(layout.inspector.right - splitter - left, narrowest, std::max(narrowest, widest)));
    return panels;
}

PanelState move_log_splitter(const MainLayout& layout, framework::Dpi dpi, PanelState panels, int top) {
    if (!layout.log || !layout.log_splitter) {
        return panels;
    }
    int splitter = layout.log_splitter->height();
    int shortest = dpi.scale(LogMinDips);
    int tallest = layout.log->bottom - splitter - layout.playlists.top - dpi.scale(PanelsMinHeightDips);
    panels.log_dips = dpi.unscale(std::clamp(layout.log->bottom - splitter - top, shortest, std::max(shortest, tallest)));
    return panels;
}

}
