#include "core/matching/MatchCandidates.h"

#include <algorithm>
#include <functional>
#include <optional>
#include <set>
#include <utility>

#include "core/matching/MatchJudgement.h"
#include "core/matching/MatchRank.h"
#include "core/matching/SongFacts.h"
#include "core/state/ListedSources.h"
#include "core/state/TrackYtmIds.h"

namespace westie::core {

namespace {

struct RankedCandidate {
    MatchCandidate candidate{};
    MatchRank rank{};
};

}

std::vector<MatchCandidate> match_candidates(
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
    // A YouTube Music id that comes again keeps its first row
    auto add = [&](MatchCandidate candidate, std::optional<bool> explicit_content, bool unless_rejected) {
        if (seen.contains(candidate.ytm_id)) {
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
        seen.insert(candidate.ytm_id);
        ranked.push_back({.candidate = std::move(candidate), .rank = match_rank(judgement, false)});
    };

    if (const std::optional<Match>& match = wanted_source.match) {
        add(
                {.ytm_id = match->ytm_id, .title = match->title, .artists = match->artists, .album = match->album, .duration = match->duration},
                match->explicit_content, false
        );
    }
    for (const std::string& key : listed_sources(state)) {
        std::optional<std::string> ytm_id = own_ytm_id(key);
        auto listed = state.sources.find(key);
        if (!ytm_id || listed == state.sources.end()) {
            continue;
        }
        const Source& song = listed->second;
        add({.ytm_id = *ytm_id, .title = song.title, .artists = song.artists, .album = song.album, .duration = song.duration}, song.explicit_content, true);
    }
    for (const SongResult& result : results) {
        add(
                {.ytm_id = result.ytm_id, .title = result.title, .artists = result.artists, .album = result.album, .duration = result.duration},
                result.explicit_content, false
        );
    }

    std::optional<ResolvedYtmId> current;
    if (auto track = state.tracks.find(wanted_source.track); track != state.tracks.end()) {
        current = resolved_ytm_id(state, track->second);
    }
    // Ties keep the order they were added in
    std::ranges::stable_sort(ranked, std::greater{}, &RankedCandidate::rank);
    std::vector<MatchCandidate> candidates;
    for (RankedCandidate& entry : ranked) {
        MatchCandidate& candidate = entry.candidate;
        candidate.current = current && current->ytm_id == candidate.ytm_id;
        if (!candidate.current) {
            candidate.track = track_with_ytm_id(state, candidate.ytm_id, wanted_source.track).value_or("");
        }
        candidates.push_back(std::move(candidate));
    }
    return candidates;
}

}
