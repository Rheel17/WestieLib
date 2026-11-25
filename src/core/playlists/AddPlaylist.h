#pragma once

#include <chrono>
#include <string>

#include "core/library/Library.h"
#include "core/log/Log.h"
#include "core/playlists/PlaylistLink.h"

namespace westie::core {

bool add_playlist(Library& library, Log& log, const PlaylistLink& link, std::string name, std::chrono::system_clock::time_point now);

}
