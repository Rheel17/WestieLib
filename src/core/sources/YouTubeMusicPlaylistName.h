#pragma once

#include <optional>
#include <stop_token>
#include <string>

#include "core/playlists/PlaylistLink.h"
#include "core/tools/YtDlp.h"

namespace westie::core {

std::optional<std::string> youtube_music_playlist_name(const YtDlp& yt_dlp, const PlaylistLink& link, const std::stop_token& stop);

}
