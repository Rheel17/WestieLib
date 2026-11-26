#pragma once

#include <optional>
#include <string>

#include "ui/inspector/TrackDetails.h"

namespace westie::ui {

struct TrackPages {
    // The audio's, as the card's Audio section shows it
    std::optional<std::wstring> youtube_music{};
    // The row's own entry when that is a Spotify one, else the track's first Spotify source
    std::optional<std::wstring> spotify{};
};

TrackPages track_pages(const TrackDetails& details);

}
