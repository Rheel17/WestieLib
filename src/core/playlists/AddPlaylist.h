#pragma once

#include <chrono>
#include <string>

#include "core/log/Log.h"
#include "core/playlists/PlaylistLink.h"
#include "core/state/StateStore.h"

namespace westie::core {

bool add_playlist(StateStore& store, Log& log, const PlaylistLink& link, std::string name, std::chrono::system_clock::time_point now);

}
