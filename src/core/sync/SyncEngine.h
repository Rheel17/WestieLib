#pragma once

#include <deque>
#include <mutex>
#include <optional>
#include <set>
#include <stop_token>
#include <string>
#include <thread>
#include <vector>

#include "core/events/EventSink.h"
#include "core/matching/Matcher.h"
#include "core/playlists/PlaylistLink.h"
#include "core/sources/PlaylistContents.h"
#include "core/sources/SpotifyUrls.h"
#include "core/state/StateStore.h"
#include "core/state/Track.h"
#include "core/tools/ToolFolder.h"
#include "core/tools/YtDlp.h"
#include "platform/HttpClient.h"
#include "platform/TextNormalizer.h"

namespace westie::core {

class SyncEngine {
public:
    SyncEngine(
            const YtDlp& yt_dlp, ToolFolder& tools, platform::HttpClient& http, const platform::TextNormalizer& normalizer, SpotifyUrls spotify,
            std::string youtube_music_api, StateStore& state, EventSink& events
    );

    void sync(const std::vector<std::string>& playlists);
    void cancel() noexcept;

private:
    struct MatchQueue {
        std::deque<std::string> waiting{};
        // Every track the run queued, so none is tried twice
        std::set<std::string> queued{};
    };

    void work(const std::stop_token& stop);
    std::optional<std::string> take_next(const std::stop_token& stop);
    bool finish(const std::stop_token& stop);
    void fetch(const std::string& playlist, const std::stop_token& stop);
    PlaylistContents fetch_contents(const PlaylistLink& link, const std::stop_token& stop);
    void show_page(const PlaylistLink& link, const PlaylistContents& so_far);
    PlaylistContents fetch_when_installed(const PlaylistLink& link, const std::stop_token& stop);
    void merge_library_matches();
    void queue_tracks(MatchQueue& queue);
    void match(const std::string& track, const std::stop_token& stop);
    void fail_match(const std::string& track, TrackFailure failure, std::string error);

    const YtDlp& yt_dlp_;
    ToolFolder& tools_;
    platform::HttpClient& http_;
    SpotifyUrls spotify_;
    const platform::TextNormalizer& normalizer_;
    Matcher matcher_;
    StateStore& state_;
    EventSink& events_;
    // Guards the queue and whether a run goes, which the UI's sync and the run's end both change
    std::mutex mutex_;
    std::deque<std::string> queue_;
    std::optional<std::string> current_;
    bool running_ = false;
    // Last, so the thread is joined before the members it uses go
    std::jthread worker_;
};

}
