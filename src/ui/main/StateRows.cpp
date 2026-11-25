#include "ui/main/StateRows.h"

#include <utility>

#include "ui/playlists/PlaylistRows.h"

namespace westie::ui {

StateRows state_rows(std::shared_ptr<const core::State> state, std::optional<std::string> playlist, const platform::TextNormalizer& normalizer) {
    StateRows rows{
        .playlists = playlist_rows(*state),
        .counts = count_tracks(*state, std::nullopt),
        .tracks = track_rows(*state, playlist, normalizer),
    };
    rows.state = std::move(state);
    rows.playlist = std::move(playlist);
    return rows;
}

}
