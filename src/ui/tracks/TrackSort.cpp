#include "ui/tracks/TrackSort.h"

#include <algorithm>
#include <compare>
#include <string>
#include <utility>

#include <windows.h>

#include "platform/win32/Win32Error.h"

namespace westie::ui {

namespace {

// As Explorer sorts names: case and accents aside, and "Track 2" before "Track 10"
std::weak_ordering compare_text(const std::wstring& left, const std::wstring& right) {
    int result = CompareStringEx(
            LOCALE_NAME_USER_DEFAULT, LINGUISTIC_IGNORECASE | SORT_DIGITSASNUMBERS, left.c_str(), static_cast<int>(left.size()), right.c_str(),
            static_cast<int>(right.size()), nullptr, nullptr, 0
    );
    if (result == 0) {
        platform::win32::throw_last_error("CompareStringEx");
    }
    return result == CSTR_LESS_THAN ? std::weak_ordering::less : result == CSTR_EQUAL ? std::weak_ordering::equivalent : std::weak_ordering::greater;
}

std::weak_ordering compare_by(const TrackRow& left, const TrackRow& right, TrackColumn column) {
    switch (column) {
    case TrackColumn::Number:    return left.position <=> right.position;
    case TrackColumn::Status:    return std::to_underlying(left.status) <=> std::to_underlying(right.status);
    case TrackColumn::Title:     return compare_text(left.title, right.title);
    case TrackColumn::Artists:   return compare_text(left.artists, right.artists);
    case TrackColumn::Album:     return compare_text(left.album, right.album);
    case TrackColumn::Playlists: return compare_text(left.playlists, right.playlists);
    case TrackColumn::Time:      return left.duration <=> right.duration;
    default:                     return std::weak_ordering::equivalent;
    }
}

bool in_order(std::weak_ordering order, framework::SortDirection direction) {
    return direction == framework::SortDirection::Ascending ? order < 0 : order > 0;
}

framework::SortDirection opposite(framework::SortDirection direction) {
    return direction == framework::SortDirection::Ascending ? framework::SortDirection::Descending : framework::SortDirection::Ascending;
}

}

TrackSort::TrackSort(TrackColumn column) : keys_{{.column = column}} {}

TrackColumn TrackSort::column() const {
    return keys_.front().column;
}

framework::SortDirection TrackSort::direction() const {
    return keys_.front().direction;
}

void TrackSort::sort_by(TrackColumn column) {
    if (column == this->column()) {
        // Every key turns round, so the rows that tie do too
        for (Key& key : keys_) {
            key.direction = opposite(key.direction);
        }
        track_key_direction_ = opposite(track_key_direction_);
        return;
    }
    std::erase_if(keys_, [column](const Key& key) { return key.column == column; });
    keys_.insert(keys_.begin(), {.column = column});
}

void TrackSort::sort(std::vector<TrackRow>& rows) const {
    std::ranges::sort(rows, [this](const TrackRow& left, const TrackRow& right) { return before(left, right); });
}

bool TrackSort::before(const TrackRow& left, const TrackRow& right) const {
    for (const Key& key : keys_) {
        if (std::weak_ordering order = compare_by(left, right, key.column); order != 0) {
            return in_order(order, key.direction);
        }
    }
    return in_order(left.key <=> right.key, track_key_direction_);
}

}
