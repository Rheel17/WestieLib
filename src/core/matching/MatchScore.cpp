#include "core/matching/MatchScore.h"

#include <algorithm>
#include <chrono>
#include <cstddef>
#include <iterator>
#include <ranges>
#include <string>
#include <vector>

namespace westie::core {

namespace {

constexpr double TitleWeight = 0.45;
constexpr double ArtistWeight = 0.35;
constexpr double LengthWeight = 0.20;
// Another of the Spotify artists matches a little less than the first
constexpr double OtherArtistWeight = 0.9;
// A name that starts the other, as Prince starts "Prince & The Revolution", is nearly the same artist; one only
// somewhere inside it is more often a tribute, as "The Hits Of Phil Collins"
constexpr double LeadingArtistWeight = 0.9;
constexpr std::chrono::milliseconds SameLength{2000};
constexpr std::chrono::milliseconds OtherLength{30000};
// A length that isn't known says nothing either way
constexpr double UnknownLengthScore = 0.5;
// A live version, a remix or a cover where the other has none is another recording
constexpr double VersionFactor = 0.5;

std::size_t common_words(std::vector<std::string> a, std::vector<std::string> b) {
    std::ranges::sort(a);
    std::ranges::sort(b);
    std::vector<std::string> common;
    std::ranges::set_intersection(a, b, std::back_inserter(common));
    return common.size();
}

double dice(const std::vector<std::string>& a, const std::vector<std::string>& b) {
    if (a.empty() || b.empty()) {
        return 0.0;
    }
    return 2.0 * static_cast<double>(common_words(a, b)) / static_cast<double>(a.size() + b.size());
}

double name_score(const std::vector<std::string>& a, const std::vector<std::string>& b) {
    if (a.empty() || b.empty()) {
        return 0.0;
    }
    bool leading = a.size() <= b.size() ? std::ranges::starts_with(b, a) : std::ranges::starts_with(a, b);
    return std::max(dice(a, b), leading ? LeadingArtistWeight : 0.0);
}

double artist_score(const MatchText& wanted, const std::vector<MatchText>& found) {
    double best = 0.0;
    for (const MatchText& artist : found) {
        best = std::max(best, name_score(wanted.words, artist.words));
    }
    return best;
}

double artists_score(const std::vector<MatchText>& wanted, const std::vector<MatchText>& found) {
    if (wanted.empty()) {
        return 0.0;
    }
    double score = artist_score(wanted.front(), found);
    for (const MatchText& other : wanted | std::views::drop(1)) {
        score = std::max(score, OtherArtistWeight * artist_score(other, found));
    }
    return score;
}

double length_score(std::chrono::milliseconds wanted, std::chrono::milliseconds found) {
    if (wanted.count() <= 0 || found.count() <= 0) {
        return UnknownLengthScore;
    }
    std::chrono::milliseconds off = wanted > found ? wanted - found : found - wanted;
    if (off <= SameLength) {
        return 1.0;
    }
    if (off >= OtherLength) {
        return 0.0;
    }
    return static_cast<double>((OtherLength - off).count()) / static_cast<double>((OtherLength - SameLength).count());
}

}

ComparableSong comparable_song(
        const platform::TextNormalizer& normalizer, std::string_view title, const std::vector<std::string>& artists, std::chrono::milliseconds duration
) {
    ComparableSong song{.title = match_text(normalizer, title), .duration = duration};
    for (const std::string& artist : artists) {
        song.artists.push_back(match_text(normalizer, artist));
    }
    return song;
}

double match_score(const ComparableSong& wanted, const ComparableSong& found) {
    double score = TitleWeight * dice(wanted.title.words, found.title.words) + ArtistWeight * artists_score(wanted.artists, found.artists) +
                   LengthWeight * length_score(wanted.duration, found.duration);
    if (wanted.title.versions != found.title.versions) {
        score *= VersionFactor;
    }
    return score;
}

}
