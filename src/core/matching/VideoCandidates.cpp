#include "core/matching/VideoCandidates.h"

#include <algorithm>
#include <functional>
#include <optional>
#include <set>
#include <utility>

#include "core/matching/MatchScore.h"
#include "core/state/ListedSources.h"
#include "core/state/TrackVideos.h"

namespace westie::core {

namespace {

// Below it, a library song is too unlike the source to be worth a row
constexpr double LibraryCandidateMinimum = 0.5;

}

std::vector<VideoCandidate> video_candidates(
        const State& state, const platform::TextNormalizer& normalizer, const std::string& source_key, const std::vector<SongResult>& results
) {
    auto source = state.sources.find(source_key);
    if (source == state.sources.end()) {
        return {};
    }
    ComparableSong wanted = comparable_song(normalizer, source->second.title, source->second.artists, source->second.duration);
    std::vector<VideoCandidate> candidates;
    std::set<std::string> seen;
    // A video that comes again keeps its first row
    auto add = [&](VideoCandidate candidate, double minimum) {
        if (seen.contains(candidate.video_id)) {
            return;
        }
        candidate.score = std::min(match_score(wanted, comparable_song(normalizer, candidate.title, candidate.artists, candidate.duration)), 1.0);
        if (candidate.score >= minimum) {
            seen.insert(candidate.video_id);
            candidates.push_back(std::move(candidate));
        }
    };

    if (const std::optional<Match>& match = source->second.match) {
        add({.video_id = match->video_id, .title = match->title, .artists = match->artists, .duration = match->duration}, 0.0);
    }
    for (const std::string& key : listed_sources(state)) {
        std::optional<std::string> video = own_video(key);
        auto listed = state.sources.find(key);
        if (!video || listed == state.sources.end()) {
            continue;
        }
        const Source& song = listed->second;
        add({.video_id = *video, .title = song.title, .artists = song.artists, .album = song.album, .duration = song.duration}, LibraryCandidateMinimum);
    }
    for (const SongResult& result : results) {
        add({.video_id = result.video_id, .title = result.title, .artists = result.artists, .album = result.album, .duration = result.duration}, 0.0);
    }

    std::optional<ResolvedVideo> current;
    if (auto track = state.tracks.find(source->second.track); track != state.tracks.end()) {
        current = resolved_video(state, track->second);
    }
    for (VideoCandidate& candidate : candidates) {
        candidate.current = current && current->video_id == candidate.video_id;
        if (!candidate.current) {
            candidate.track = track_with_video(state, candidate.video_id, source->second.track).value_or("");
        }
    }
    // Ties keep the order they were added in
    std::ranges::stable_sort(candidates, std::greater{}, &VideoCandidate::score);
    return candidates;
}

}
