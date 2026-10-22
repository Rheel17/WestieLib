#pragma once

#include <string>

#include "core/sources/SourceType.h"
#include "ui/TrackCounts.h"

namespace westie::ui {

struct PlaylistRow {
    core::SourceType type = core::SourceType::YouTubeMusic;
    std::wstring name{};
    TrackCounts counts{};
};

}
