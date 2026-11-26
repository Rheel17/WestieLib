#include "ui/main/MatchChoices.h"

#include <algorithm>
#include <utility>

#include "core/state/TrackYtmIds.h"
#include "ui/tracks/TrackReview.h"

namespace westie::ui {

bool needs_choice(const core::State& state, const std::string& track) {
    auto found = state.tracks.find(track);
    if (found == state.tracks.end() || core::source_to_match(state, found->second) == nullptr) {
        return false;
    }
    // A search that failed is tried again on the next update
    bool no_match = found->second.status == core::TrackStatus::Failed && found->second.failure == core::TrackFailure::NoMatch;
    return no_match || shows_review(state, found->second);
}

void MatchChoices::start_update() {
    offered_.clear();
}

void MatchChoices::add(const core::State& state, const std::string& track) {
    if (!offered_.contains(track) && needs_choice(state, track)) {
        offered_.insert(track);
        waiting_.push_back(track);
    }
}

std::optional<std::string> MatchChoices::take(const core::State& state) {
    while (!waiting_.empty()) {
        std::string track = std::move(waiting_.front());
        waiting_.pop_front();
        if (needs_choice(state, track)) {
            return track;
        }
    }
    return std::nullopt;
}

bool MatchChoices::any_waiting(const core::State& state) const {
    return std::ranges::any_of(waiting_, [&](const std::string& track) { return needs_choice(state, track); });
}

void MatchChoices::clear() {
    waiting_.clear();
}

}
