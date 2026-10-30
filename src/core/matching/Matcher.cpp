#include "core/matching/Matcher.h"

#include <algorithm>
#include <format>
#include <string_view>
#include <utility>
#include <vector>

#include "core/matching/MatchJudgement.h"
#include "core/matching/MatchQueries.h"
#include "core/matching/MatchRank.h"
#include "core/matching/SongFacts.h"
#include "core/matching/YouTubeMusicSearch.h"

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

Matcher::Matcher(platform::HttpClient& http, const platform::TextNormalizer& normalizer, std::string api_url) :
        http_(http), normalizer_(normalizer), api_url_(std::move(api_url)) {}

std::expected<Match, std::string> Matcher::match(const Source& source, const std::set<std::string>& known_videos, const std::stop_token& stop) const {
    SongFacts wanted = song_facts(normalizer_, source.title, source.artists, source.album, source.duration, source.explicit_content);
    // Each video once, in the order the searches found them
    std::vector<Candidate> pool;
    std::vector<std::string> searched;
    for (const std::string& query : match_queries(source)) {
        std::vector<SongResult> results = search_youtube_music_songs(http_, api_url_, query, stop);
        bool accepted = false;
        for (SongResult& result : results) {
            if (std::ranges::contains(pool, result.video_id, [](const Candidate& candidate) { return candidate.song.video_id; })) {
                continue;
            }
            SongFacts found = song_facts(normalizer_, result.title, result.artists, result.album, result.duration, result.explicit_content);
            MatchJudgement judgement = judge_match(wanted, found);
            accepted = accepted || judgement.verdict == MatchVerdict::Accept;
            MatchRank rank = match_rank(judgement, known_videos.contains(result.video_id));
            pool.push_back({.song = std::move(result), .judgement = std::move(judgement), .rank = rank});
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
        .video_id = best->song.video_id,
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
