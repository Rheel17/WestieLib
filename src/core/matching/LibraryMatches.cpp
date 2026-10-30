#include "core/matching/LibraryMatches.h"

#include <algorithm>
#include <map>
#include <optional>
#include <set>
#include <utility>

#include "core/matching/MatchJudgement.h"
#include "core/matching/MatchRank.h"
#include "core/matching/MatchWords.h"
#include "core/matching/SongFacts.h"
#include "core/sources/SourceType.h"
#include "core/state/ListedSources.h"
#include "core/state/TrackVideos.h"

namespace westie::core {

namespace {

struct ListedSong {
    std::string video_id{};
    std::string track{};
    const Source* source = nullptr;
    SongFacts song{};
};

// The letters of the name, and of the name with its lead and with tail, which may or may not be credits
std::set<std::string> name_keys(const TitleParts& title) {
    return {letters_of(title.name), letters_of(whole_name(title))};
}

std::vector<ListedSong> listed_songs(const State& state, const platform::TextNormalizer& normalizer, const std::set<std::string>& listed) {
    std::vector<ListedSong> songs;
    for (const std::string& key : listed) {
        std::optional<std::string> video = own_video(key);
        auto source = state.sources.find(key);
        if (!video || source == state.sources.end() || !state.tracks.contains(source->second.track)) {
            continue;
        }
        const Source& listed_source = source->second;
        songs.push_back({
            .video_id = std::move(*video),
            .track = listed_source.track,
            .source = &listed_source,
            .song = song_facts(
                    normalizer, listed_source.title, listed_source.artists, listed_source.album, listed_source.duration, listed_source.explicit_content
            ),
        });
    }
    return songs;
}

bool has_listed_youtube_music_source(const Track& track, const std::set<std::string>& listed) {
    return std::ranges::any_of(track.sources, [&](const std::string& key) { return own_video(key) && listed.contains(key); });
}

}

std::vector<LibraryMatch> library_matches(const State& state, const platform::TextNormalizer& normalizer) {
    std::set<std::string> listed = listed_sources(state);
    std::vector<ListedSong> songs = listed_songs(state, normalizer, listed);
    // By name, so a song is held only against the ones that can be it
    std::map<std::string, std::vector<const ListedSong*>> by_name;
    for (const ListedSong& song : songs) {
        for (const std::string& key : name_keys(song.song.title)) {
            by_name[key].push_back(&song);
        }
    }

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
        const Source& wanted_source = source->second;
        SongFacts wanted = song_facts(
                normalizer, wanted_source.title, wanted_source.artists, wanted_source.album, wanted_source.duration, wanted_source.explicit_content
        );
        std::set<const ListedSong*> candidates;
        for (const std::string& name : name_keys(wanted.title)) {
            if (auto found = by_name.find(name); found != by_name.end()) {
                candidates.insert(found->second.begin(), found->second.end());
            }
        }

        const ListedSong* best = nullptr;
        MatchJudgement best_judgement;
        MatchRank best_rank;
        for (const ListedSong* candidate : candidates) {
            if (candidate->track == key) {
                continue;
            }
            MatchJudgement judgement = judge_match(wanted, candidate->song);
            MatchRank rank = match_rank(judgement, false);
            if (best == nullptr || rank > best_rank) {
                best = candidate;
                best_judgement = std::move(judgement);
                best_rank = rank;
            }
        }
        // Only an entry that is the same recording beyond doubt takes the place of a search
        if (best == nullptr || best_judgement.verdict != MatchVerdict::Accept) {
            continue;
        }
        matches.push_back({
            .track = key,
            .match = {
                .video_id = best->video_id,
                .confidence = best_judgement.confidence,
                .title = best->source->title,
                .artists = best->source->artists,
                .album = best->source->album,
                .duration = best->source->duration,
                .explicit_content = best->source->explicit_content,
            },
        });
    }
    return matches;
}

}
