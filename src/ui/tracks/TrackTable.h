#pragma once

#include <windows.h>
#include <commctrl.h>

#include "ui/framework/Dpi.h"
#include "ui/framework/ListView.h"
#include "ui/framework/MessageFont.h"

namespace westie::ui {

// Title is sub-item 0: column 0 holds the item's text, can't be right-aligned and can't be removed
enum class TrackColumn { Title, Number, Status, Artists, Album, Playlists, Time, Source, Match, File };

struct TrackSort {
    TrackColumn column = TrackColumn::Title;
    framework::SortDirection direction = framework::SortDirection::Ascending;
};

class TrackTable {
public:
    explicit TrackTable(HWND parent);

    HWND hwnd() const noexcept;
    void apply_dpi(framework::Dpi dpi, const framework::MessageFont& font);
    void show_scope(bool playlist);
    LRESULT on_notify(NMHDR& header);

private:
    void add_number_column();
    void sort_by(int column_index);
    void show_sort();
    static void show_cell(LVITEMW& item);

    framework::ListView list_;
    TrackSort sort_;
};

}
