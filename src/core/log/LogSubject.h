#pragma once

#include <string>

#include "core/state/State.h"

namespace westie::core {

struct LogSubject {
    std::string playlist{};
    std::string track{};
    // The track's title, else the playlist's name, as it was when the line was written
    std::string name{};
};

LogSubject playlist_subject(const State& state, const std::string& playlist);
LogSubject track_subject(const State& state, const std::string& track);

}
