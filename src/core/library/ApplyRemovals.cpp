#include "core/library/ApplyRemovals.h"

namespace westie::core {

namespace {

bool applies(const Track& track, RemovalDecision decision) {
    switch (decision) {
    case RemovalDecision::Keep:   return track.status == TrackStatus::Orphaned;
    case RemovalDecision::Remove: return track.status == TrackStatus::Orphaned || track.status == TrackStatus::Kept;
    }
    return false;
}

}

// The decisions it applied: a track that came back into a playlist meanwhile, or is gone, stays as it is
std::map<std::string, RemovalDecision> apply_removals(State& state, const std::map<std::string, RemovalDecision>& decisions) {
    std::map<std::string, RemovalDecision> applied;
    for (const auto& [key, decision] : decisions) {
        auto track = state.tracks.find(key);
        if (track == state.tracks.end() || !applies(track->second, decision)) {
            continue;
        }
        applied.emplace(key, decision);
        if (decision == RemovalDecision::Keep) {
            track->second.status = TrackStatus::Kept;
            continue;
        }
        for (const std::string& source : track->second.sources) {
            state.sources.erase(source);
        }
        state.tracks.erase(track);
    }
    return applied;
}

}