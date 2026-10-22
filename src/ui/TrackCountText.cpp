#include "ui/TrackCountText.h"

#include <format>

namespace westie::ui {

std::wstring track_count_text(int count) {
    return std::format(L"{} {}", count, count == 1 ? L"track" : L"tracks");
}

std::wstring synced_count_text(const TrackCounts& counts) {
    return std::format(L"{} · {} synced", track_count_text(counts.total), counts.synced);
}

}
