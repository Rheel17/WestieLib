#include "core/library/FileChanges.h"

#include <map>
#include <utility>

#include "core/library/LibraryFileName.h"
#include "core/state/EffectiveMetadata.h"

namespace westie::core {

namespace {

std::map<std::filesystem::path, std::string> file_owners(const State& state) {
    std::map<std::filesystem::path, std::string> owners;
    for (const auto& [key, track] : state.tracks) {
        if (!track.file.empty()) {
            owners.emplace(track.file, key);
        }
    }
    return owners;
}

std::string stem_of(const State& state, const Track& track) {
    return library_file_stem(effective_metadata(state, track), track.ytm_id);
}

}

TrackTags track_tags(const State& state, const Track& track) {
    TrackMetadata metadata = effective_metadata(state, track);
    return {
        .title = std::move(metadata.title),
        .artists = std::move(metadata.artists),
        .album = std::move(metadata.album),
        .year = metadata.year,
        .sources = track.sources,
        .ytm_id = track.ytm_id,
    };
}

FileChanges file_changes(const State& before, const State& after) {
    FileChanges changes;
    std::map<std::filesystem::path, std::string> owners_before = file_owners(before);
    std::map<std::filesystem::path, std::string> owners_after = file_owners(after);
    for (const auto& [file, key] : owners_before) {
        if (!owners_after.contains(file)) {
            changes.dropped.push_back({.file = file, .track = key});
        }
    }
    for (const auto& [file, key] : owners_after) {
        auto owner = owners_before.find(file);
        // A file that is new, or that a download just replaced, was tagged by it
        if (owner == owners_before.end()) {
            continue;
        }
        const Track& old_track = before.tracks.at(owner->second);
        const Track& track = after.tracks.at(key);
        if (old_track.ytm_id != track.ytm_id) {
            continue;
        }
        TrackTags tags = track_tags(after, track);
        if (tags == track_tags(before, old_track)) {
            continue;
        }
        std::string stem = stem_of(after, track);
        bool renamed = stem != stem_of(before, old_track);
        changes.retagged.push_back({.file = file, .track = key, .tags = std::move(tags), .stem = renamed ? std::optional(std::move(stem)) : std::nullopt});
    }
    return changes;
}

}
