#include "core/state/SplitSource.h"

#include <algorithm>
#include <utility>
#include <vector>

namespace westie::core {

void split_source(State& state, const std::string& source_key) {
    auto source = state.sources.find(source_key);
    if (source == state.sources.end()) {
        return;
    }
    auto track = state.tracks.find(source->second.track);
    if (track == state.tracks.end() || track->second.sources.size() < 2) {
        return;
    }

    if (track->first != source_key) {
        std::erase(track->second.sources, source_key);
        state.tracks[source_key] = {.sources = {source_key}, .status = track->second.status, .added_at = track->second.added_at};
        source->second.track = source_key;
        return;
    }

    // A track key never changes, so the track keeps the source whose key it has, and the others go
    std::vector<std::string> others = track->second.sources;
    std::erase(others, source_key);
    std::string others_key = others.front();
    Track rest = track->second;
    rest.sources = std::move(others);
    rest.video_override.clear();
    for (const std::string& key : rest.sources) {
        if (auto moved = state.sources.find(key); moved != state.sources.end()) {
            moved->second.track = others_key;
        }
    }
    track->second.sources = {source_key};
    state.tracks[others_key] = std::move(rest);
}

}
