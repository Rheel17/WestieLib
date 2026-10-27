#pragma once

#include <vector>

#include "ui/framework/ListView.h"
#include "ui/tracks/TrackRows.h"

namespace westie::ui {

// Title is sub-item 0: column 0 holds the item's text, can't be right-aligned and can't be removed
enum class TrackColumn { Title, Number, Status, Artists, Album, Playlists, Time, Source, Match, File };

class TrackSort {
public:
    explicit TrackSort(TrackColumn column);

    TrackColumn column() const;
    framework::SortDirection direction() const;
    void sort_by(TrackColumn column);
    void sort(std::vector<TrackRow>& rows) const;

private:
    struct Key {
        TrackColumn column = TrackColumn::Title;
        framework::SortDirection direction = framework::SortDirection::Ascending;
    };

    bool before(const TrackRow& left, const TrackRow& right) const;

    // The column clicked last comes first, so rows that tie on it keep the order the clicks before gave them
    std::vector<Key> keys_;
    framework::SortDirection track_key_direction_ = framework::SortDirection::Ascending;
};

}
