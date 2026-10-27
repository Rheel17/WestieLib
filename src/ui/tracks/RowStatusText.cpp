#include "ui/tracks/RowStatusText.h"

#include <format>

#include "ui/tracks/StatusTabText.h"

namespace westie::ui {

std::wstring row_status_word(const TrackRow& row) {
    return row.duplicate_of ? L"Duplicate" : status_word(row.status);
}

std::wstring row_status_tip(const TrackRow& row) {
    if (row.duplicate_of) {
        return std::format(L"Duplicate of track #{} · {}", row.duplicate_of->position, status_word(row.status));
    }
    return status_word(row.status);
}

std::wstring duplicate_explanation(const RowOriginal& original) {
    return std::format(L"Duplicate of track #{} in this playlist.", original.position);
}

}
