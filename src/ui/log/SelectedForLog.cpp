#include "ui/log/SelectedForLog.h"

namespace westie::ui {

LogSelection selected_for_log(const core::State& state, const std::optional<std::string>& playlist, const std::vector<TrackRow>& selected_rows) {
    LogSelection selection{.playlist = playlist};
    if (playlist) {
        if (auto found = state.playlists.find(*playlist); found != state.playlists.end()) {
            for (const std::string& source : found->second.sources) {
                if (auto listed = state.sources.find(source); listed != state.sources.end()) {
                    selection.playlist_tracks.insert(listed->second.track);
                }
            }
        }
    }
    for (const TrackRow& row : selected_rows) {
        selection.tracks.insert(row.track);
    }
    return selection;
}

}
