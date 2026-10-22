#pragma once

#include <stop_token>

#include "core/playlists/PlaylistLink.h"
#include "core/sources/PlaylistContents.h"
#include "core/tools/YtDlp.h"

namespace westie::core {

PlaylistContents fetch_youtube_music_playlist(const YtDlp& yt_dlp, const PlaylistLink& link, const std::stop_token& stop);

}
