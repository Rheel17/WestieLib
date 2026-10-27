#include "core/matching/Matcher.h"

#include <algorithm>
#include <format>
#include <string_view>
#include <utility>
#include <vector>

#include "core/matching/MatchScore.h"
#include "core/matching/MatchText.h"
#include "core/matching/YouTubeMusicSearch.h"

namespace westie::core {

namespace {

// Below it, a result shares too little with the track to be the same recording: a matching title and length alone, as
// a cover by another artist has, stay under it
constexpr double MinimumScore = 0.7;
// Toward the video that another track already has, so the same song from both sources tends to become one track
constexpr double KnownVideoBonus = 0.05;

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

std::string query_of(std::string_view artists, std::string_view title) {
    return artists.empty() ? std::string(title) : std::format("{} {}", artists, title);
}

std::vector<std::string> queries_for(const Source& source) {
    std::vector<std::string> queries = {query_of(joined(source.artists, " "), source.title)};
    std::string bare = query_of(source.artists.empty() ? std::string() : source.artists.front(), bare_title(source.title));
    if (bare != queries.front()) {
        queries.push_back(std::move(bare));
    }
    return queries;
}

}

Matcher::Matcher(platform::HttpClient& http, const platform::TextNormalizer& normalizer, std::string api_url) :
        http_(http), normalizer_(normalizer), api_url_(std::move(api_url)) {}

std::expected<Match, std::string> Matcher::match(const Source& source, const std::set<std::string>& known_videos, const std::stop_token& stop) const {
    ComparableSong wanted = comparable_song(normalizer_, source.title, source.artists, source.duration);
    std::vector<std::string> searched;
    for (const std::string& query : queries_for(source)) {
        std::vector<SongResult> results = search_youtube_music_songs(http_, api_url_, query, stop);
        const SongResult* best = nullptr;
        double best_score = 0.0;
        for (const SongResult& result : results) {
            double score = match_score(wanted, comparable_song(normalizer_, result.title, result.artists, result.duration));
            if (known_videos.contains(result.video_id)) {
                score += KnownVideoBonus;
            }
            // Ties go to YouTube Music's order
            if (best == nullptr || score > best_score) {
                best = &result;
                best_score = score;
            }
        }
        if (best != nullptr && best_score >= MinimumScore) {
            return Match{
                .video_id = best->video_id,
                .confidence = std::min(best_score, 1.0),
                .title = best->title,
                .artists = best->artists,
                .duration = best->duration,
            };
        }
        searched.push_back(std::format("Searched \"{}\": {}", query, results.empty() ? "no results" : "no usable results"));
    }
    return std::unexpected(joined(searched, "\n"));
}

}
