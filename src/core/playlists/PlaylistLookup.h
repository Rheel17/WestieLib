#pragma once

#include <optional>
#include <stop_token>
#include <string>
#include <thread>

#include "core/events/EventSink.h"
#include "core/playlists/PlaylistLink.h"
#include "core/tools/YtDlp.h"
#include "platform/HttpClient.h"

namespace westie::core {

class PlaylistLookup {
public:
    PlaylistLookup(const YtDlp& yt_dlp, platform::HttpClient& http, std::string spotify_url, EventSink& events);

    void look_up(PlaylistLink link);
    void stop() noexcept;

private:
    void work(const std::stop_token& stop, const PlaylistLink& link);
    std::optional<std::string> name(const PlaylistLink& link, const std::stop_token& stop) const;

    const YtDlp& yt_dlp_;
    platform::HttpClient& http_;
    std::string spotify_url_;
    EventSink& events_;
    // Last, so the thread is joined before the members it uses go
    std::jthread worker_;
};

}
