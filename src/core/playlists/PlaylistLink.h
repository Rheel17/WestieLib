#pragma once

#include <optional>
#include <string>
#include <string_view>

#include "core/sources/SourceType.h"

namespace westie::core {

struct PlaylistLink {
    SourceType type = SourceType::YouTubeMusic;
    std::string id{};

    std::string key() const;
    std::string url() const;
};

std::optional<PlaylistLink> parse_playlist_link(std::string_view text);

}
