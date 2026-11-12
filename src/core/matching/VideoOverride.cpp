#include "core/matching/VideoOverride.h"

#include <format>
#include <utility>

#include "core/log/LogLevel.h"
#include "core/log/LogSubject.h"
#include "core/matching/ConfirmMatch.h"
#include "core/state/MatchReview.h"
#include "core/state/MergeTracks.h"
#include "core/state/SplitSource.h"
#include "core/state/TrackVideos.h"

namespace westie::core {

std::optional<std::string> set_video_override(StateStore& store, Log& log, const std::string& source_key, const std::string& video_id) {
    std::optional<std::string> survivor;
    std::optional<LogSubject> subject;
    std::string text;
    store.update([&](State& state) {
        subject.reset();
        auto source = state.sources.find(source_key);
        if (source == state.sources.end() || !state.tracks.contains(source->second.track)) {
            return;
        }
        subject = track_subject(state, source->second.track);
        std::optional<ResolvedVideo> current = resolved_video(state, state.tracks.at(source->second.track));
        if (current && current->video_id == video_id) {
            // Choosing the match that may have been wrong says it is right
            if (needs_review(state, state.tracks.at(source->second.track))) {
                survivor = confirm_match(state, source->second.track);
                text = "Confirmed the match";
            } else {
                subject.reset();
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
        subject->track = key;
        text = std::format("Chose the video {}", video_id);
        survivor = track_with_video(state, video_id, key);
        if (survivor) {
            merge_tracks(state, *survivor, key);
        }
    });
    if (subject) {
        // This merge has no event of its own, so the line is about the track that is left
        if (survivor) {
            subject->track = *survivor;
            text += ", and merged into the track that has that video";
        }
        log.write(LogLevel::Info, std::move(text), std::move(*subject));
    }
    return survivor;
}

}
