#pragma once

#include <functional>
#include <vector>

#include <windows.h>

#include "ui/TrackCounts.h"
#include "ui/commands/CommandId.h"
#include "ui/framework/Dpi.h"
#include "ui/framework/FontSet.h"
#include "ui/framework/Panel.h"
#include "ui/framework/Rect.h"
#include "ui/playlists/AllTracksRow.h"
#include "ui/playlists/PlaylistList.h"
#include "ui/playlists/PlaylistRow.h"

namespace westie::ui {

class PlaylistPanel final : public framework::Panel {
public:
    PlaylistPanel(HWND parent, std::function<void(CommandId)> on_command);

    void apply_dpi(framework::Dpi dpi, const framework::FontSet& fonts);
    void set_track_counts(const TrackCounts& counts);
    void set_playlists(std::vector<PlaylistRow> rows);

protected:
    LRESULT handle_message(UINT message, WPARAM wparam, LPARAM lparam) override;

private:
    void layout();
    void paint() const;

    AllTracksRow all_tracks_;
    PlaylistList list_;
    framework::Rect rule_;
};

}
