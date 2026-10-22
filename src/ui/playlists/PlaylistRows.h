#pragma once

#include <vector>

#include "core/state/State.h"
#include "ui/playlists/PlaylistRow.h"

namespace westie::ui {

std::vector<PlaylistRow> playlist_rows(const core::State& state);

}
