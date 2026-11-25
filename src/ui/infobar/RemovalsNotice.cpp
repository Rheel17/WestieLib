#include "ui/infobar/RemovalsNotice.h"

#include <algorithm>
#include <format>

namespace westie::ui {

void RemovalsNotice::update(const core::State& state) {
    removed_.clear();
    for (const auto& [key, track] : state.tracks) {
        if (track.status == core::TrackStatus::Orphaned) {
            removed_.insert(key);
        }
    }
    if (removed_.empty() || std::ranges::includes(dismissed_, removed_)) {
        message_.reset();
        return;
    }
    message_ = removed_.size() == 1 ? std::wstring(L"1 track is no longer in any playlist.")
                                    : std::format(L"{} tracks are no longer in any playlist.", removed_.size());
}

void RemovalsNotice::dismiss() {
    dismissed_ = removed_;
    message_.reset();
}

const std::optional<std::wstring>& RemovalsNotice::message() const noexcept {
    return message_;
}

}
