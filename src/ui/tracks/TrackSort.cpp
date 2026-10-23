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

}

void sort_track_rows(std::vector<TrackRow>& rows, const TrackSort& sort) {
    bool descending = sort.direction == framework::SortDirection::Descending;
    std::ranges::sort(rows, [&](const TrackRow& left, const TrackRow& right) {
        std::weak_ordering order = compare_by(left, right, sort.column);
        if (order != 0) {
            return descending ? order > 0 : order < 0;
        }
        // Rows that tie keep one order whichever way the column sorts
        if (std::weak_ordering title = compare_text(left.title, right.title); title != 0) {
            return title < 0;
        }
        return left.key < right.key;
    });
}

}
