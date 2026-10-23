#pragma once

#include <optional>
#include <string>
#include <vector>

#include <windows.h>

#include "ui/framework/Dpi.h"
#include "ui/framework/FontSet.h"
#include "ui/framework/Label.h"
#include "ui/framework/Panel.h"
#include "ui/playlists/PlaylistRow.h"
#include "ui/tracks/FilterBar.h"
#include "ui/tracks/StatusFilter.h"
#include "ui/tracks/TrackRows.h"
#include "ui/tracks/TrackTable.h"

namespace westie::ui {

class TrackPanel final : public framework::Panel {
public:
    explicit TrackPanel(HWND parent);

    void apply_dpi(framework::Dpi dpi, const framework::FontSet& fonts);
    void show_tracks(const std::optional<PlaylistRow>& playlist, std::vector<TrackRow> rows, int library_total);

protected:
    LRESULT handle_message(UINT message, WPARAM wparam, LPARAM lparam) override;

private:
    void layout();
    void paint() const;
    LRESULT on_notify(NMHDR& header);
    void on_filter_change();
    void update_clear();
    void clear_status();
    void show_tab_change(StatusTab before);
    void show_rows();
    void show_result_line();

    FilterBar filter_;
    StatusFilter status_;
    framework::Label result_;
    TrackTable table_;
    // The scope's playlist, or nothing for All tracks
    std::optional<std::string> playlist_;
    std::optional<std::wstring> playlist_name_;
    // Every row of the scope, before the source and the tab narrow them
    std::vector<TrackRow> scope_rows_;
    int shown_ = 0;
    int library_total_ = 0;
    int result_height_ = 0;
    int rule_top_ = 0;
};

}
