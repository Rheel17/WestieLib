#pragma once

#include <map>
#include <string>

#include "core/state/Playlist.h"
#include "core/state/Source.h"
#include "core/state/Track.h"

namespace westie::core {

struct State {
    std::map<std::string, Playlist> playlists{};
    std::map<std::string, Source> sources{};
    std::map<std::string, Track> tracks{};
};

}
