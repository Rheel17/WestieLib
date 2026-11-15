#pragma once

#include <string>
#include <vector>

namespace westie::core {

struct QueuedTrack {
    std::string track{};
    // The playlists that list it, so the UI can count it under each of them even after it merges
    std::vector<std::string> playlists{};
};

struct TracksQueued {
    std::vector<QueuedTrack> tracks{};
};

}
