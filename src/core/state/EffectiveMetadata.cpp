#include "core/state/EffectiveMetadata.h"

#include <array>

#include "core/sources/SourceType.h"
#include "core/state/TrackVideos.h"

namespace westie::core {

namespace {

// Spotify's metadata is the cleanest: separate artists, a proper album
constexpr std::array SourceOrder = {SourceType::Spotify, SourceType::YouTubeMusic};

}

TrackMetadata effective_metadata(const State& state, const Track& track) {
    TrackMetadata metadata;
    for (SourceType type : SourceOrder) {
        const std::string* key = first_source_of(track, type);
        if (key == nullptr) {
            continue;
        }
        auto found = state.sources.find(*key);
        if (found == state.sources.end()) {
            continue;
        }
        const Source& source = found->second;
        // Each field from the first source that has it
        if (metadata.title.empty()) {
            metadata.title = source.title;
        }
        if (metadata.artists.empty()) {
            metadata.artists = source.artists;
        }
        if (metadata.album.empty()) {
            metadata.album = source.album;
        }
        if (metadata.duration.count() <= 0) {
            metadata.duration = source.duration;
        }
    }
    return metadata;
}

}
