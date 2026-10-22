#pragma once

#include <string>

#include "core/sources/SourceType.h"
#include "ui/TrackCounts.h"

namespace westie::ui {

struct PlaylistRow {
    std::string key{};
    core::SourceType type = core::SourceType::YouTubeMusic;
    std::wstring name{};
    std::wstring id{};
    std::wstring url{};
    TrackCounts counts{};
};

}
