#include "ui/main/MainLayout.h"

#include <algorithm>

namespace westie::ui {

namespace {

constexpr int SplitterWidthDips = 5;

}

MainLayout compute_main_layout(framework::Rect client, framework::Dpi dpi, const PanelWidths& widths) {
    int splitter = dpi.scale(SplitterWidthDips);
    int playlists_min = dpi.scale(PlaylistsMinDips);
    int inspector_min = dpi.scale(InspectorMinDips);
    int playlists = std::max(dpi.scale(widths.playlists_dips), playlists_min);
    int inspector = std::max(dpi.scale(widths.inspector_dips), inspector_min);

    int overflow = playlists + inspector + 2 * splitter + dpi.scale(TracksMinDips) - client.width();
    int inspector_cut = std::clamp(overflow, 0, inspector - inspector_min);
    inspector -= inspector_cut;
    playlists -= std::clamp(overflow - inspector_cut, 0, playlists - playlists_min);

    MainLayout layout;
    layout.playlists = client.take_left(playlists);
    layout.playlists_splitter = client.take_left(splitter);
    layout.inspector = client.take_right(inspector);
    layout.inspector_splitter = client.take_right(splitter);
    layout.tracks = client;
    return layout;
}

PanelWidths move_playlists_splitter(const MainLayout& layout, framework::Dpi dpi, PanelWidths widths, int left) {
    int narrowest = dpi.scale(PlaylistsMinDips);
    int widest = layout.tracks.right - dpi.scale(TracksMinDips) - layout.playlists_splitter.width() - layout.playlists.left;
    widths.playlists_dips = dpi.unscale(std::clamp(left - layout.playlists.left, narrowest, std::max(narrowest, widest)));
    return widths;
}

PanelWidths move_inspector_splitter(const MainLayout& layout, framework::Dpi dpi, PanelWidths widths, int left) {
    int splitter = layout.inspector_splitter.width();
    int narrowest = dpi.scale(InspectorMinDips);
    int widest = layout.inspector.right - splitter - layout.playlists_splitter.right - dpi.scale(TracksMinDips);
    widths.inspector_dips = dpi.unscale(std::clamp(layout.inspector.right - splitter - left, narrowest, std::max(narrowest, widest)));
    return widths;
}

}
