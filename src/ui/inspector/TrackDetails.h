#pragma once

#include <optional>
#include <string>
#include <vector>

#include "core/sources/SourceType.h"
#include "core/state/State.h"
#include "ui/tracks/TrackRows.h"

namespace westie::ui {

struct TrackSource {
    core::SourceType type = core::SourceType::YouTubeMusic;
    std::wstring url{};
};

struct TrackPlaylist {
    std::string key{};
    std::wstring name{};
};

struct TrackDetails {
    TrackRow row{};
    // The first source of each kind, in the order of AllSourceTypes
    std::vector<TrackSource> sources{};
    // In the left panel's order
    std::vector<TrackPlaylist> playlists{};
};

std::optional<TrackDetails> track_details(const core::State& state, const TrackRow& row);

}
