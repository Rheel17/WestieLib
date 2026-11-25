#pragma once

#include <functional>
#include <map>
#include <optional>
#include <string>
#include <vector>

#include <windows.h>
#include <commctrl.h>

#include "ui/framework/Dpi.h"
#include "ui/framework/IconResource.h"
#include "ui/framework/ListView.h"
#include "ui/framework/MessageFont.h"
#include "ui/RingIcons.h"
#include "ui/tracks/StatusCounts.h"
#include "ui/tracks/TrackActivity.h"
#include "ui/tracks/TrackRows.h"
#include "ui/tracks/TrackSort.h"

namespace westie::ui {

class TrackTable {
public:
    TrackTable(HWND parent, std::function<void()> on_selection_change);

    HWND hwnd() const noexcept;
    void apply_dpi(framework::Dpi dpi, const framework::MessageFont& font);
    void show_scope(bool playlist);
    std::map<TrackColumn, int> column_widths() const;
    void set_column_widths(const std::map<TrackColumn, int>& width_dips);
    void set_rows(std::vector<TrackRow> rows);
    void set_activities(TrackActivities activities);
    void scroll_to_top();
    void clear_selection();
    std::vector<TrackRow> selected_rows() const;
    LRESULT on_notify(NMHDR& header);

private:
    struct StatusIcon {
        // None for a duplicate
        std::optional<StatusTab> status{};
        framework::IconResource icon;
    };

    // By track key, so it stays with the tracks when the rows change or move
    struct Selection {
        std::vector<std::string> keys{};
        std::optional<std::string> focused{};
    };

    void on_item_changed(const NMLISTVIEW& change) const;
    void add_number_column();
    void sort_by(int column_index);
    void show_sort();
    Selection selection() const;
    void restore(const Selection& selection);
    void show_cell(LVITEMW& item) const;
    std::wstring cell_tip(int item, int column_index) const;
    LRESULT on_custom_draw(const NMLVCUSTOMDRAW& draw) const;
    void draw_status(HDC dc, int item) const;
    const framework::IconResource* status_icon(const TrackRow& row) const;

    std::function<void()> on_selection_change_;
    // Every column's, so a column that comes back, like #, has the width it had
    std::map<TrackColumn, int> width_dips_;
    framework::ListView list_;
    TrackSort sort_;
    std::vector<TrackRow> rows_;
    std::vector<StatusIcon> status_icons_;
    // What the update does with the tracks right now, by track key
    TrackActivities activities_;
    std::optional<RingIcons> rings_;
    std::optional<framework::IconResource> tool_wait_icon_;
};

}
