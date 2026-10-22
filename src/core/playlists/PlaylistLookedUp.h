#pragma once

#include <string>

namespace westie::core {

enum class PlaylistLookupOutcome { Found, Unavailable, ToolMissing, Failed };

struct PlaylistLookedUp {
    std::string key{};
    PlaylistLookupOutcome outcome = PlaylistLookupOutcome::Found;
    std::string name{};
    std::string error{};
};

}
