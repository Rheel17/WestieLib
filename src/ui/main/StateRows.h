#pragma once

#include <memory>
#include <optional>
#include <string>
#include <vector>

#include "core/state/State.h"
#include "platform/TextNormalizer.h"
#include "ui/TrackCounts.h"
#include "ui/playlists/PlaylistRow.h"
#include "ui/tracks/TrackRows.h"

namespace westie::ui {

struct StateRows {
    std::shared_ptr<const core::State> state{};
    // The scope the track rows are of: a playlist's key, or nothing for All tracks
    std::optional<std::string> playlist{};
    std::vector<PlaylistRow> playlists{};
    TrackCounts counts{};
    std::vector<TrackRow> tracks{};
};

StateRows state_rows(std::shared_ptr<const core::State> state, std::optional<std::string> playlist, const platform::TextNormalizer& normalizer);

}
