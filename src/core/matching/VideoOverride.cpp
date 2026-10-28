#include "core/matching/VideoOverride.h"

#include "core/matching/ConfirmMatch.h"
#include "core/state/MatchReview.h"
#include "core/state/MergeTracks.h"
#include "core/state/SplitSource.h"
#include "core/state/TrackVideos.h"

namespace westie::core {

std::optional<std::string> set_video_override(StateStore& store, const std::string& source_key, const std::string& video_id) {
    std::optional<std::string> survivor;
    store.update([&](State& state) {
        auto source = state.sources.find(source_key);
        if (source == state.sources.end() || !state.tracks.contains(source->second.track)) {
            return;
        }
        std::optional<ResolvedVideo> current = resolved_video(state, state.tracks.at(source->second.track));
        if (current && current->video_id == video_id) {
            // Choosing the match that may have been wrong says it is right
            if (needs_review(state, state.tracks.at(source->second.track))) {
                survivor = confirm_match(state, source->second.track);
            }
            return;
        }
        // Another entry of the track keeps the video it has
        if (!current || current->source != source_key) {
            split_source(state, source_key);
        }

        std::string key = source->second.track;
        Track& track = state.tracks.at(key);
        track.video_override = video_id;
        if (track.status == TrackStatus::Failed) {
            track.status = TrackStatus::Pending;
        }
        track.failure = TrackFailure::None;
        track.error.clear();
        survivor = track_with_video(state, video_id, key);
        if (survivor) {
            merge_tracks(state, *survivor, key);
        }
    });
    return survivor;
}

}
