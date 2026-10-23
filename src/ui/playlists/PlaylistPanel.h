#pragma once

#include <optional>
#include <string>
#include <vector>

#include <windows.h>

#include "ui/TrackCounts.h"
#include "ui/framework/Dpi.h"
#include "ui/framework/FontSet.h"
#include "ui/framework/Panel.h"
#include "ui/framework/Rect.h"
#include "ui/playlists/AllTracksRow.h"
#include "ui/playlists/PlaylistEvents.h"
#include "ui/playlists/PlaylistList.h"
#include "ui/playlists/PlaylistRow.h"

namespace westie::ui {

class PlaylistPanel final : public framework::Panel {
public:
    PlaylistPanel(HWND parent, PlaylistEvents events);

    void apply_dpi(framework::Dpi dpi, const framework::FontSet& fonts);
    void set_track_counts(const TrackCounts& counts);
    void set_playlists(std::vector<PlaylistRow> rows);
    void set_sync_enabled(bool enabled);
    void show_selection(const std::optional<std::string>& key);

protected:
    LRESULT handle_message(UINT message, WPARAM wparam, LPARAM lparam) override;

private:
    void select_all_tracks() const;
    void layout();
    void paint() const;

    PlaylistEvents events_;
    AllTracksRow all_tracks_;
    PlaylistList list_;
    framework::Rect rule_;
};

}
