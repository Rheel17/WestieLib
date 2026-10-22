#pragma once

#include <chrono>

#include "core/playlists/PlaylistLink.h"
#include "core/state/StateStore.h"

namespace westie::core {

bool add_playlist(StateStore& store, const PlaylistLink& link, std::chrono::system_clock::time_point now);

}
