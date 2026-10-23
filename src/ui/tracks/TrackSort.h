#pragma once

#include <vector>

#include "ui/framework/ListView.h"
#include "ui/tracks/TrackRows.h"

namespace westie::ui {

// Title is sub-item 0: column 0 holds the item's text, can't be right-aligned and can't be removed
enum class TrackColumn { Title, Number, Status, Artists, Album, Playlists, Time, Source, Match, File };

struct TrackSort {
    TrackColumn column = TrackColumn::Title;
    framework::SortDirection direction = framework::SortDirection::Ascending;
};

void sort_track_rows(std::vector<TrackRow>& rows, const TrackSort& sort);

}
