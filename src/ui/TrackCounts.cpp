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
    // As the track table has rows: the library's tracks, a playlist's entries with a duplicate under its track's status
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
    // A source the playlist lists twice counts once
    std::set<std::string> sources(listed->second.sources.begin(), listed->second.sources.end());
    for (const std::string& key : sources) {
        auto source = state.sources.find(key);
        if (source == state.sources.end()) {
            continue;
        }
        if (auto track = state.tracks.find(source->second.track); track != state.tracks.end()) {
            count(counts, track->second);
        }
    }
    return counts;
}

}
