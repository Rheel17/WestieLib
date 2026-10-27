#include "core/state/MergeTracks.h"

#include <algorithm>

namespace westie::core {

void merge_tracks(State& state, const std::string& survivor, const std::string& merged) {
    auto into = state.tracks.find(survivor);
    auto from = state.tracks.find(merged);
    if (into == state.tracks.end() || from == state.tracks.end() || into == from) {
        return;
    }
    for (const std::string& key : from->second.sources) {
        if (!std::ranges::contains(into->second.sources, key)) {
            into->second.sources.push_back(key);
        }
        if (auto source = state.sources.find(key); source != state.sources.end()) {
            source->second.track = survivor;
        }
    }
    state.tracks.erase(from);
}

}
