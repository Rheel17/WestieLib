#include "core/state/TrackVideos.h"

#include <algorithm>
#include <optional>
#include <string>

#include "core/sources/SourceType.h"

namespace westie::core {

namespace {

const std::string* first_source_of(const Track& track, SourceType type) {
    auto found = std::ranges::find(track.sources, std::optional(type), [](const std::string& key) { return source_type_of_key(key); });
    return found == track.sources.end() ? nullptr : &*found;
}

}

std::optional<ResolvedVideo> resolved_video(const State& state, const Track& track) {
    // The playlist owner chose a YouTube Music source's video
    if (const std::string* key = first_source_of(track, SourceType::YouTubeMusic)) {
        return ResolvedVideo{.video_id = key->substr(source_prefix(SourceType::YouTubeMusic).size() + 1), .source = *key};
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

}
