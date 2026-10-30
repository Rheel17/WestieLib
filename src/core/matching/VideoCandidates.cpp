#include "core/matching/VideoCandidates.h"

#include <algorithm>
#include <functional>
#include <optional>
#include <set>
#include <utility>

#include "core/matching/MatchJudgement.h"
#include "core/matching/MatchRank.h"
#include "core/matching/SongFacts.h"
#include "core/state/ListedSources.h"
#include "core/state/TrackVideos.h"

namespace westie::core {

namespace {

struct RankedCandidate {
    VideoCandidate candidate{};
    MatchRank rank{};
};

}

std::vector<VideoCandidate> video_candidates(
        const State& state, const platform::TextNormalizer& normalizer, const std::string& source_key, const std::vector<SongResult>& results
) {
    auto source = state.sources.find(source_key);
    if (source == state.sources.end()) {
        return {};
    }
    const Source& wanted_source = source->second;
    SongFacts wanted = song_facts(
            normalizer, wanted_source.title, wanted_source.artists, wanted_source.album, wanted_source.duration, wanted_source.explicit_content
    );
    std::vector<RankedCandidate> ranked;
    std::set<std::string> seen;
    // A video that comes again keeps its first row
    auto add = [&](VideoCandidate candidate, std::optional<bool> explicit_content, bool unless_rejected) {
        if (seen.contains(candidate.video_id)) {
            return;
        }
        MatchJudgement judgement = judge_match(
                wanted, song_facts(normalizer, candidate.title, candidate.artists, candidate.album, candidate.duration, explicit_content)
        );
        if (unless_rejected && judgement.verdict == MatchVerdict::Reject) {
            return;
        }
        candidate.verdict = judgement.verdict;
        candidate.mismatch = judgement.mismatch;
        candidate.doubts = judgement.doubts;
        seen.insert(candidate.video_id);
        ranked.push_back({.candidate = std::move(candidate), .rank = match_rank(judgement, false)});
    };

    if (const std::optional<Match>& match = wanted_source.match) {
        add(
                {.video_id = match->video_id, .title = match->title, .artists = match->artists, .album = match->album, .duration = match->duration},
                match->explicit_content, false
        );
    }
    for (const std::string& key : listed_sources(state)) {
        std::optional<std::string> video = own_video(key);
        auto listed = state.sources.find(key);
        if (!video || listed == state.sources.end()) {
            continue;
        }
        const Source& song = listed->second;
        add({.video_id = *video, .title = song.title, .artists = song.artists, .album = song.album, .duration = song.duration}, song.explicit_content, true);
    }
    for (const SongResult& result : results) {
        add(
                {.video_id = result.video_id, .title = result.title, .artists = result.artists, .album = result.album, .duration = result.duration},
                result.explicit_content, false
        );
    }

    std::optional<ResolvedVideo> current;
    if (auto track = state.tracks.find(wanted_source.track); track != state.tracks.end()) {
        current = resolved_video(state, track->second);
    }
    // Ties keep the order they were added in
    std::ranges::stable_sort(ranked, std::greater{}, &RankedCandidate::rank);
    std::vector<VideoCandidate> candidates;
    for (RankedCandidate& entry : ranked) {
        VideoCandidate& candidate = entry.candidate;
        candidate.current = current && current->video_id == candidate.video_id;
        if (!candidate.current) {
            candidate.track = track_with_video(state, candidate.video_id, wanted_source.track).value_or("");
        }
        candidates.push_back(std::move(candidate));
    }
    return candidates;
}

}
