#include "ui/tracks/StatusTabText.h"

#include <format>

namespace westie::ui {

namespace {

const wchar_t* status_word(StatusTab tab) {
    switch (tab) {
    case StatusTab::Synced:      return L"Synced";
    case StatusTab::Waiting:     return L"Waiting";
    case StatusTab::NeedsReview: return L"Needs review";
    case StatusTab::Failed:      return L"Failed";
    case StatusTab::Removed:     return L"Removed";
    case StatusTab::Kept:        return L"Kept";
    default:                     return L"All";
    }
}

}

int status_count(const StatusCounts& counts, StatusTab tab) {
    switch (tab) {
    case StatusTab::Synced:      return counts.synced;
    case StatusTab::Waiting:     return counts.waiting;
    case StatusTab::NeedsReview: return counts.needs_review;
    case StatusTab::Failed:      return counts.failed;
    case StatusTab::Removed:     return counts.removed;
    case StatusTab::Kept:        return counts.kept;
    default:                     return counts.all;
    }
}

std::wstring status_tab_text(const StatusCounts& counts, StatusTab tab) {
    return std::format(L"{} {}", status_word(tab), status_count(counts, tab));
}

}
