#include "core/state/TrackVideos.h"

#include <algorithm>
#include <optional>
#include <ranges>
#include <string>
#include <utility>

#include "core/sources/SourceType.h"
#include "core/state/MatchReview.h"

namespace westie::core {

const std::string* first_source_of(const Track& track, SourceType type) {
    auto found = std::ranges::find(track.sources, std::optional(type), [](const std::string& key) { return source_type_of_key(key); });
    return found == track.sources.end() ? nullptr : &*found;
}

std::optional<std::string> own_video(std::string_view source_key) {
    if (source_type_of_key(source_key) != SourceType::YouTubeMusic) {
        return std::nullopt;
    }
    return std::string(source_key.substr(source_prefix(SourceType::YouTubeMusic).size() + 1));
}

std::optional<ResolvedVideo> resolved_video(const State& state, const Track& track) {
    // The playlist owner chose a YouTube Music source's video
    if (const std::string* key = first_source_of(track, SourceType::YouTubeMusic)) {
        return ResolvedVideo{.video_id = own_video(*key).value_or(""), .source = *key};
    }
    const std::string* key = first_source_of(track, SourceType::Spotify);
    if (key == nullptr) {
        return std::nullopt;
    }
    auto source = state.sources.find(*key);
    if (source == state.sources.end() || !source->second.match) {
        return std::nullopt;
    }
    return ResolvedVideo{.video_id = source->second.match->video_id, .source = *key};
}

std::optional<std::string> track_with_video(const State& state, std::string_view video_id, std::string_view except) {
    for (const auto& [key, track] : state.tracks) {
        // A match that may be wrong takes no other track's sources in
        if (key == except || needs_review(state, track)) {
            continue;
        }
        if (std::optional<ResolvedVideo> video = resolved_video(state, track); video && video->video_id == video_id) {
            return key;
        }
    }
    return std::nullopt;
}

std::set<std::string> track_videos(const State& state) {
    std::set<std::string> videos;
    for (const Track& track : state.tracks | std::views::values) {
        if (needs_review(state, track)) {
            continue;
        }
        if (std::optional<ResolvedVideo> video = resolved_video(state, track)) {
            videos.insert(std::move(video->video_id));
        }
    }
    return videos;
}

}
