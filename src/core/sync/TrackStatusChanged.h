#pragma once

#include <string>

#include "core/state/Track.h"

namespace westie::core {

struct TrackStatusChanged {
    std::string track{};
    TrackStatus status = TrackStatus::Pending;
    TrackFailure failure = TrackFailure::None;
    std::string error{};
};

}
