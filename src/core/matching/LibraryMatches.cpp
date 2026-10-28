#include "core/matching/LibraryMatches.h"

#include <algorithm>
#include <map>
#include <optional>
#include <set>

#include "core/matching/MatchScore.h"
#include "core/sources/SourceType.h"
#include "core/state/ListedSources.h"
#include "core/state/TrackVideos.h"

namespace westie::core {

namespace {

// The same recording beyond doubt: a playlist entry takes the place of a search result only then
constexpr double LibraryMinimum = 0.9;

struct ListedSong {
    std::string video_id{};
    std::string track{};
    const Source* source = nullptr;
    ComparableSong song{};
};

std::string words_key(const MatchText& text) {
    std::string key;
    for (const std::string& word : text.words) {
        if (!key.empty()) {
            key += ' ';
        }
        key += word;
    }
    return key;
}

std::map<std::string, std::vector<ListedSong>> listed_songs(
        const State& state, const platform::TextNormalizer& normalizer, const std::set<std::string>& listed
) {
    // By bare title, so a song is held only against the ones that can be it
    std::map<std::string, std::vector<ListedSong>> songs;
    for (const std::string& key : listed) {
        std::optional<std::string> video = own_video(key);
        auto source = state.sources.find(key);
        if (!video || source == state.sources.end() || !state.tracks.contains(source->second.track)) {
            continue;
        }
        ComparableSong song = comparable_song(normalizer, source->second.title, source->second.artists, source->second.duration);
        std::string title = words_key(song.title);
        songs[title].push_back({.video_id = std::move(*video), .track = source->second.track, .source = &source->second, .song = std::move(song)});
    }
    return songs;
}

bool has_listed_youtube_music_source(const Track& track, const std::set<std::string>& listed) {
    return std::ranges::any_of(track.sources, [&](const std::string& key) { return own_video(key) && listed.contains(key); });
}

}

std::vector<LibraryMatch> library_matches(const State& state, const platform::TextNormalizer& normalizer) {
    std::set<std::string> listed = listed_sources(state);
    std::map<std::string, std::vector<ListedSong>> songs = listed_songs(state, normalizer, listed);

    std::vector<LibraryMatch> matches;
    for (const auto& [key, track] : state.tracks) {
        bool chosen = !track.video_override.empty() || has_listed_youtube_music_source(track, listed);
        if (track.status == TrackStatus::Orphaned || track.status == TrackStatus::Kept || chosen) {
            continue;
        }
        const std::string* spotify = first_source_of(track, SourceType::Spotify);
        auto source = spotify == nullptr ? state.sources.end() : state.sources.find(*spotify);
        if (source == state.sources.end()) {
            continue;
        }
        ComparableSong wanted = comparable_song(normalizer, source->second.title, source->second.artists, source->second.duration);
        auto candidates = songs.find(words_key(wanted.title));
        if (candidates == songs.end()) {
            continue;
        }

        const ListedSong* best = nullptr;
        double best_score = 0.0;
        for (const ListedSong& candidate : candidates->second) {
            double score = match_score(wanted, candidate.song);
            if (candidate.track != key && score > best_score) {
                best = &candidate;
                best_score = score;
            }
        }
        if (best == nullptr || best_score < LibraryMinimum) {
            continue;
        }
        matches.push_back({
            .track = key,
            .match = {
                .video_id = best->video_id,
                .confidence = std::min(best_score, 1.0),
                .title = best->source->title,
                .artists = best->source->artists,
                .duration = best->source->duration,
            },
        });
    }
    return matches;
}

}
