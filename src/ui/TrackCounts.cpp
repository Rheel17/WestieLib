#include "ui/TrackCounts.h"

#include <ranges>
#include <set>

namespace westie::ui {

namespace {

void count(TrackCounts& counts, const core::Track& track) {
    ++counts.total;
    switch (track.status) {
    case core::TrackStatus::Downloaded: ++counts.synced; break;
    case core::TrackStatus::Pending:    ++counts.waiting; break;
    case core::TrackStatus::Failed:     ++counts.failed; break;
    default:                            break;
    }
}

}

TrackCounts count_tracks(const core::State& state, const std::optional<std::string>& playlist) {
    TrackCounts counts;
    if (!playlist) {
        for (const core::Track& track : state.tracks | std::views::values) {
            count(counts, track);
        }
        return counts;
    }

    auto listed = state.playlists.find(*playlist);
    if (listed == state.playlists.end()) {
        return counts;
    }
    // A track the playlist lists twice, or through two sources, counts once
    std::set<std::string> keys;
    for (const std::string& source : listed->second.sources) {
        if (auto entry = state.sources.find(source); entry != state.sources.end()) {
            keys.insert(entry->second.track);
        }
    }
    for (const std::string& key : keys) {
        if (auto track = state.tracks.find(key); track != state.tracks.end()) {
            count(counts, track->second);
        }
    }
    return counts;
}

}
