#include "ui/tracks/TrackReview.h"

#include "core/state/MatchReview.h"

namespace westie::ui {

bool shows_review(const core::State& state, const core::Track& track) {
    // Failed, Removed and Kept say more about what the user can do
    bool waiting_or_synced = track.status == core::TrackStatus::Pending || track.status == core::TrackStatus::Downloaded;
    return waiting_or_synced && core::needs_review(state, track);
}

}
