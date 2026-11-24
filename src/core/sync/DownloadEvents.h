#pragma once

#include <string>
#include <vector>

#include "core/sync/TracksQueued.h"

namespace westie::core {

struct DownloadsQueued {
    std::vector<QueuedTrack> tracks{};
};

struct TrackDownloading {
    std::string track{};
};

struct TrackProgress {
    std::string track{};
    int percent = 0;
};

}
