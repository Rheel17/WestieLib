#include "core/matching/Matcher.h"

#include <algorithm>
#include <cstddef>
#include <exception>
#include <format>
#include <string_view>
#include <utility>
#include <vector>

#include "core/log/LogLevel.h"
#include "core/matching/MatchJudgement.h"
#include "core/matching/MatchQueries.h"
#include "core/matching/MatchRank.h"
#include "core/matching/RefusedSearch.h"
#include "core/matching/SearchLogText.h"
#include "core/matching/SongFacts.h"
#include "platform/Cancelled.h"

namespace westie::core {

namespace {

struct Candidate {
    SongResult song{};
    MatchJudgement judgement{};
    MatchRank rank{};
};

std::string joined(const std::vector<std::string>& parts, std::string_view separator) {
    std::string text;
    for (const std::string& part : parts) {
        if (!text.empty()) {
            text += separator;
        }
        text += part;
    }
    return text;
}

}

Matcher::Matcher(YouTubeMusicSearch& search, const platform::TextNormalizer& normalizer, Log& log) : search_(search), normalizer_(normalizer), log_(log) {}

std::expected<Match, std::string> Matcher::match(
        const Source& source, const std::set<std::string>& known_ytm_ids, const LogSubject& subject, const std::stop_token& stop
) const {
    SongFacts wanted = song_facts(normalizer_, source.title, source.artists, source.album, source.duration, source.explicit_content);
    // Each YouTube Music id once, in the order the searches found them
    std::vector<Candidate> pool;
    std::vector<std::string> searched;
    for (const std::string& query : match_queries(source)) {
        log_.write(LogLevel::Debug, search_started_text(query), subject);
        std::vector<SongResult> results;
        try {
            results = search_.songs(query, stop);
        } catch (const platform::Cancelled&) {
            throw;
        } catch (const std::exception& error) {
            bool refused = refused_search(error);
            log_.write(refused ? LogLevel::Warning : LogLevel::Debug, search_failed_text(query, error.what(), refused), subject);
            throw;
        }
        bool accepted = false;
        // This search's results, as places in the pool
        std::vector<std::size_t> found;
        for (SongResult& result : results) {
            auto known = std::ranges::find(pool, result.ytm_id, [](const Candidate& candidate) { return candidate.song.ytm_id; });
            if (known != pool.end()) {
                found.push_back(static_cast<std::size_t>(known - pool.begin()));
                continue;
            }
            SongFacts facts = song_facts(normalizer_, result.title, result.artists, result.album, result.duration, result.explicit_content);
            MatchJudgement judgement = judge_match(wanted, facts);
            accepted = accepted || judgement.verdict == MatchVerdict::Accept;
            MatchRank rank = match_rank(judgement, known_ytm_ids.contains(result.ytm_id));
            pool.push_back({.song = std::move(result), .judgement = std::move(judgement), .rank = rank});
            found.push_back(pool.size() - 1);
        }
        log_.write(LogLevel::Debug, search_finished_text(query, results.size()), subject);
        // The pool may have this search's best from an earlier one
        if (!found.empty()) {
            const Candidate& best = pool[*std::ranges::max_element(found, {}, [&](std::size_t index) { return pool[index].rank; })];
            log_.write(LogLevel::Debug, best_result_text(query, best.song.title, best.song.artists, best.judgement.confidence), subject);
        }
        searched.push_back(std::format("Searched \"{}\": {}", query, results.empty() ? "no results" : "no usable results"));
        if (accepted) {
            break;
        }
    }

    // The first of the best, so ties go to YouTube Music's order
    auto best = std::ranges::max_element(pool, {}, &Candidate::rank);
    if (best == pool.end() || best->judgement.verdict == MatchVerdict::Reject) {
        return std::unexpected(joined(searched, "\n"));
    }
    return Match{
        .ytm_id = best->song.ytm_id,
        .confidence = best->judgement.confidence,
        .doubts = best->judgement.doubts,
        .title = best->song.title,
        .artists = best->song.artists,
        .album = best->song.album,
        .duration = best->song.duration,
        .explicit_content = best->song.explicit_content,
    };
}

}
