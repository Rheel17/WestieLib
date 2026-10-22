#pragma once

#include <map>
#include <string>

#include "core/state/Playlist.h"

namespace westie::core {

struct State {
    std::map<std::string, Playlist> playlists{};
};

}
