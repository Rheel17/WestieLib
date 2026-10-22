#pragma once

#include <functional>

#include <windows.h>

#include "ui/TrackCounts.h"
#include "ui/commands/CommandId.h"
#include "ui/framework/Button.h"
#include "ui/framework/Dpi.h"
#include "ui/framework/FontSet.h"
#include "ui/framework/Label.h"
#include "ui/framework/Panel.h"
#include "ui/framework/Rect.h"
#include "ui/playlists/AllTracksRow.h"

namespace westie::ui {

class PlaylistPanel final : public framework::Panel {
public:
    PlaylistPanel(HWND parent, std::function<void(CommandId)> on_command);

    void apply_dpi(framework::Dpi dpi, const framework::FontSet& fonts);
    void set_track_counts(const TrackCounts& counts);
    void set_has_playlists(bool has_playlists);

protected:
    LRESULT handle_message(UINT message, WPARAM wparam, LPARAM lparam) override;

private:
    void layout();
    void paint() const;
    void on_command(WPARAM wparam) const;

    std::function<void(CommandId)> on_command_;
    AllTracksRow all_tracks_;
    framework::Label empty_text_;
    framework::Button add_button_;
    framework::Rect rule_;
    int empty_text_height_ = 0;
    bool has_playlists_ = false;
};

}
