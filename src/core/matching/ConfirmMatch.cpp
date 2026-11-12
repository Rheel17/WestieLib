#include "core/matching/ConfirmMatch.h"

#include <utility>

#include "core/log/LogLevel.h"
#include "core/log/LogSubject.h"
#include "core/sources/SourceType.h"
#include "core/state/MergeTracks.h"
#include "core/state/TrackVideos.h"

namespace westie::core {

std::optional<std::string> confirm_match(State& state, const std::string& track_key) {
    auto track = state.tracks.find(track_key);
    if (track == state.tracks.end()) {
        return std::nullopt;
    }
    std::optional<ResolvedVideo> video = resolved_video(state, track->second);
    if (!video || video->overridden || source_type_of_key(video->source) != SourceType::Spotify) {
        return std::nullopt;
    }
    std::optional<Match>& match = state.sources.at(video->source).match;
    match->confirmed = true;

    // As the Matcher would have, had it been sure: the track that had the video first survives
    std::optional<std::string> survivor = track_with_video(state, video->video_id, track_key);
    if (survivor) {
        merge_tracks(state, *survivor, track_key);
    }
    return survivor;
}

std::optional<std::string> confirm_match(StateStore& store, Log& log, const std::string& track_key) {
    LogSubject subject = track_subject(*store.snapshot(), track_key);
    std::optional<std::string> survivor;
    store.update([&](State& state) { survivor = confirm_match(state, track_key); });
    // This merge has no event of its own, so the line is about the track that is left
    if (survivor) {
        subject.track = *survivor;
    }
    log.write(LogLevel::Info, survivor ? "Confirmed the match, and merged into the track that has that video" : "Confirmed the match", std::move(subject));
    return survivor;
}

}
