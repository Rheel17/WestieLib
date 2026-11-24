#pragma once

#include <chrono>
#include <condition_variable>
#include <deque>
#include <filesystem>
#include <mutex>
#include <optional>
#include <set>
#include <stop_token>
#include <string>
#include <string_view>
#include <thread>
#include <vector>

#include "core/events/EventSink.h"
#include "core/log/Log.h"
#include "core/log/LogSubject.h"
#include "core/matching/Matcher.h"
#include "core/matching/YouTubeMusicSearch.h"
#include "core/playlists/PlaylistLink.h"
#include "core/settings/SettingsStore.h"
#include "core/sources/PlaylistContents.h"
#include "core/sources/SpotifyUrls.h"
#include "core/state/Match.h"
#include "core/state/StateStore.h"
#include "core/state/Track.h"
#include "core/sync/DownloadPool.h"
#include "core/sync/TrackDownload.h"
#include "core/tools/ToolFolder.h"
#include "core/tools/YtDlp.h"
#include "platform/FileOps.h"
#include "platform/HttpClient.h"
#include "platform/TextNormalizer.h"

namespace westie::core {

// What the download step needs beyond what the fetch and the match use
struct Downloads {
    const TrackDownload& download;
    platform::FileOps& files;
    const SettingsStore& settings;
    std::filesystem::path work_folder{};
};

class SyncEngine {
public:
    SyncEngine(
            const YtDlp& yt_dlp, ToolFolder& tools, platform::HttpClient& http, const platform::TextNormalizer& normalizer, SpotifyUrls spotify,
            YouTubeMusicSearch& youtube_music, StateStore& state, EventSink& events, Log& log, Downloads downloads
    );

    void sync(const std::vector<std::string>& playlists);
    void cancel() noexcept;

private:
    struct QueuedMatch {
        std::string track{};
        int tries = 0;
        // A track whose search YouTube Music refused waits a while before it is tried again
        std::chrono::steady_clock::time_point not_before{};
    };

    struct MatchQueue {
        std::deque<QueuedMatch> waiting{};
        // Every track the run queued, so none is queued twice
        std::set<std::string> queued{};
    };

    void work(const std::stop_token& stop);
    std::optional<std::string> take_next(const std::stop_token& stop);
    std::optional<QueuedMatch> take_track(MatchQueue& queue, const std::stop_token& stop);
    void queue_downloads(std::optional<DownloadPool>& pool, const std::stop_token& stop);
    bool wait_for_downloads(DownloadPool& pool, const std::stop_token& stop);
    void clear_work_folder();
    bool finish(const std::stop_token& stop);
    void fetch(const std::string& playlist, const std::stop_token& stop);
    PlaylistContents fetch_contents(const PlaylistLink& link, const std::stop_token& stop);
    void show_page(const PlaylistLink& link, const PlaylistContents& so_far);
    PlaylistContents fetch_when_installed(const PlaylistLink& link, const std::stop_token& stop);
    void merge_library_matches();
    void queue_tracks(MatchQueue& queue);
    void match(QueuedMatch queued, MatchQueue& queue, const std::stop_token& stop);
    void fail_match(const std::string& track, TrackFailure failure, std::string error, int tries);
    void log_match(LogSubject subject, const Match& match, std::string_view from, bool merged);

    const YtDlp& yt_dlp_;
    ToolFolder& tools_;
    platform::HttpClient& http_;
    SpotifyUrls spotify_;
    const platform::TextNormalizer& normalizer_;
    Matcher matcher_;
    StateStore& state_;
    EventSink& events_;
    Log& log_;
    Downloads downloads_;
    // Guards the queue and whether a run goes, which the UI's sync and the run's end both change
    std::mutex mutex_;
    // Wakes a run that waits for a track to be due when a playlist is queued
    std::condition_variable_any wake_;
    std::deque<std::string> queue_;
    std::optional<std::string> current_;
    bool running_ = false;
    // Last, so the thread is joined before the members it uses go
    std::jthread worker_;
};

}
