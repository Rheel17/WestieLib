#include "ui/infobar/FetchFailureNotice.h"

#include <format>

namespace westie::ui {

void FetchFailureNotice::update(const std::vector<PlaylistRow>& rows) {
    shown_.reset();
    message_.reset();
    // The rows come in the order the playlists were added, and the first failure shows
    for (const PlaylistRow& row : rows) {
        if (!row.fetch_failed_at) {
            continue;
        }
        Failure failure{row.key, *row.fetch_failed_at};
        if (!dismissed_.contains(failure)) {
            shown_ = std::move(failure);
            message_ = std::format(L"Couldn’t fetch “{}”. It may be private or deleted.", row.name);
            return;
        }
    }
}

void FetchFailureNotice::dismiss() {
    if (shown_) {
        dismissed_.insert(*shown_);
    }
}

const std::optional<std::wstring>& FetchFailureNotice::message() const noexcept {
    return message_;
}

}
