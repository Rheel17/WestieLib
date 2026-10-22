#include "core/sync/SyncEngine.h"

#include <algorithm>
#include <chrono>
#include <cstdint>
#include <exception>
#include <utility>

#include "core/sources/SourceType.h"
#include "core/sources/YouTubeMusicPlaylist.h"
#include "core/sync/ApplyFetch.h"
#include "core/sync/PlaylistFetchFailed.h"
#include "core/sync/PlaylistFetched.h"
#include "core/sync/PlaylistFetching.h"
#include "core/sync/SyncFinished.h"
#include "core/sync/SyncQueued.h"
#include "core/tools/ToolMissing.h"
#include "platform/Cancelled.h"

namespace westie::core {

SyncEngine::SyncEngine(const YtDlp& yt_dlp, ToolFolder& tools, StateStore& state, EventSink& events) :
        yt_dlp_(yt_dlp), tools_(tools), state_(state), events_(events) {}

void SyncEngine::sync(const std::vector<std::string>& playlists) {
    std::unique_lock lock(mutex_);
    if (running_ && worker_.get_stop_token().stop_requested()) {
        // A cancelled run would drop the new playlists with its queue, so they wait for it to end and start their own
        lock.unlock();
        worker_.join();
        lock.lock();
    }

    SyncQueued queued;
    for (const std::string& playlist : playlists) {
        // Fetching a Spotify playlist doesn't exist yet
        if (source_type_of_key(playlist) != SourceType::YouTubeMusic || current_ == playlist || std::ranges::contains(queue_, playlist)) {
            continue;
        }
        queue_.push_back(playlist);
        queued.playlists.push_back(playlist);
    }
    if (queued.playlists.empty()) {
        return;
    }
    // Under the lock, so the run can't report on these playlists before the UI knows they're queued
    events_.emit(std::move(queued));
    if (running_) {
        return;
    }
    running_ = true;
    lock.unlock();
    // Assigning joins the run before, which has ended already
    worker_ = std::jthread([this](std::stop_token stop) { work(stop); });
}

void SyncEngine::cancel() noexcept {
    worker_.request_stop();
}

void SyncEngine::work(const std::stop_token& stop) {
    while (std::optional<std::string> playlist = take_next(stop)) {
        fetch(*playlist, stop);
    }
}

std::optional<std::string> SyncEngine::take_next(const std::stop_token& stop) {
    std::scoped_lock lock(mutex_);
    current_.reset();
    if (stop.stop_requested() || queue_.empty()) {
        queue_.clear();
        running_ = false;
        // Under the lock, so a sync that starts the next run comes after this in the events
        events_.emit(SyncFinished{.cancelled = stop.stop_requested()});
        return std::nullopt;
    }
    current_ = std::move(queue_.front());
    queue_.pop_front();
    return current_;
}

void SyncEngine::fetch(const std::string& playlist, const std::stop_token& stop) {
    std::optional<PlaylistLink> link = playlist_link_of_key(playlist);
    if (!link) {
        return;
    }
    // An exception that leaves a thread ends the process, so it becomes the playlist's error instead
    try {
        PlaylistContents contents = fetch_when_installed(*link, stop);
        state_.update([&](State& state) { apply_fetch(state, playlist, std::move(contents), std::chrono::system_clock::now()); });
        events_.emit(PlaylistFetched{.playlist = playlist});
    } catch (const platform::Cancelled&) {
        return;
    } catch (const std::exception& error) {
        PlaylistFetchFailed failed{.playlist = playlist, .error = error.what()};
        try {
            state_.update([&](State& state) { apply_fetch_failure(state, playlist, failed.error, std::chrono::system_clock::now()); });
        } catch (const std::exception&) {
            // A state that can't be saved now keeps the playlist's previous error; the event still tells the UI
        }
        events_.emit(std::move(failed));
    }
}

PlaylistContents SyncEngine::fetch_when_installed(const PlaylistLink& link, const std::stop_token& stop) {
    events_.emit(PlaylistFetching{.playlist = link.key()});
    while (true) {
        std::uint64_t installs = tools_.installs(ToolPackage::YtDlp);
        try {
            return fetch_youtube_music_playlist(yt_dlp_, link, stop);
        } catch (const ToolMissing& missing) {
            events_.emit(PlaylistFetching{.playlist = link.key(), .waiting_for = {missing.package()}});
            if (!tools_.wait_for_install_after(missing.package(), installs, stop)) {
                throw platform::Cancelled();
            }
            events_.emit(PlaylistFetching{.playlist = link.key()});
        }
    }
}

}
