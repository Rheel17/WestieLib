#pragma once

#include <functional>
#include <stop_token>

#include "core/playlists/PlaylistLink.h"
#include "core/sources/PlaylistContents.h"
#include "core/sources/SpotifyUrls.h"
#include "platform/HttpClient.h"

namespace westie::core {

PlaylistContents fetch_spotify_playlist(
        platform::HttpClient& http, const SpotifyUrls& urls, const PlaylistLink& link, const std::function<void(const PlaylistContents& so_far)>& on_page,
        const std::stop_token& stop
);

}
