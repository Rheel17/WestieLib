#include "core/matching/ConfirmMatch.h"

#include "core/sources/SourceType.h"
#include "core/state/MergeTracks.h"
#include "core/state/TrackVideos.h"

namespace westie::core {

std::optional<std::string> confirm_match(StateStore& store, const std::string& track_key) {
    std::optional<std::string> survivor;
    store.update([&](State& state) {
        auto track = state.tracks.find(track_key);
        if (track == state.tracks.end()) {
            return;
        }
        std::optional<ResolvedVideo> video = resolved_video(state, track->second);
        if (!video || source_type_of_key(video->source) != SourceType::Spotify) {
            return;
        }
        std::optional<Match>& match = state.sources.at(video->source).match;
        match->confirmed = true;

        // As the Matcher would have, had it been sure: the track that had the video first survives
        survivor = track_with_video(state, video->video_id, track_key);
        if (survivor) {
            merge_tracks(state, *survivor, track_key);
        }
    });
    return survivor;
}

}
