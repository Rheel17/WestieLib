#include "core/state/MergeTracks.h"

#include <algorithm>
#include <optional>

#include "core/state/PruneTracks.h"
#include "core/state/TrackYtmIds.h"

namespace westie::core {

namespace {

// The survivor keeps its own file, unless it has none or only the other one was made from the id they share
bool takes_file(const State& state, const Track& survivor, const Track& merged) {
    if (merged.file.empty()) {
        return false;
    }
    if (survivor.file.empty()) {
        return true;
    }
    std::optional<ResolvedYtmId> shared = resolved_ytm_id(state, survivor);
    return shared && merged.ytm_id == shared->ytm_id && survivor.ytm_id != shared->ytm_id;
}

void take_file(const State& state, Track& survivor, const Track& merged) {
    survivor.file = merged.file;
    survivor.ytm_id = merged.ytm_id;
    survivor.status = merged.status;
    survivor.failure = merged.failure;
    survivor.error = merged.error;
    // The merged track may have left every playlist, but the survivor is in one
    bool left = survivor.status == TrackStatus::Orphaned || survivor.status == TrackStatus::Kept;
    if (left && is_listed(state, survivor)) {
        survivor.status = returning_status(state, survivor);
    }
}

}

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
    if (takes_file(state, into->second, from->second)) {
        take_file(state, into->second, from->second);
    }
    state.tracks.erase(from);
}

}
