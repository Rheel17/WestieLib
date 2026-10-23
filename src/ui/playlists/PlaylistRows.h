#pragma once

#include <optional>
#include <string>
#include <vector>

#include "core/state/State.h"
#include "ui/playlists/PlaylistRow.h"
#include "ui/sync/SyncBoard.h"

namespace westie::ui {

std::vector<PlaylistRow> playlist_rows(const core::State& state);
std::optional<PlaylistRow> playlist_row(const core::State& state, const std::string& key);
void mark_runs(std::vector<PlaylistRow>& rows, const SyncBoard& board);

}
