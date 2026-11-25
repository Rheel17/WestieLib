#include "core/sync/SyncEngine.h"

#include <algorithm>
#include <chrono>
#include <cstddef>
#include <cstdint>
#include <exception>
#include <expected>
#include <format>
#include <memory>
#include <ranges>
#include <set>
#include <utility>

#include "core/log/LogLevel.h"
#include "core/log/LogText.h"
#include "core/matching/LibraryMatches.h"
#include "core/matching/MatchText.h"
#include "core/matching/RefusedSearch.h"
#include "core/sources/SourceType.h"
#include "core/sources/SpotifyPlaylist.h"
#include "core/sources/YouTubeMusicPlaylist.h"
#include "core/state/MatchReview.h"
#include "core/state/TrackYtmIds.h"
#include "core/sync/ApplyDownload.h"
#include "core/sync/ApplyFetch.h"
#include "core/sync/ApplyMatch.h"
#include "core/sync/PlaylistFetchFailed.h"
#include "core/sync/PlaylistFetched.h"
#include "core/sync/PlaylistFetching.h"
#include "core/sync/PlaylistPageFetched.h"
#include "core/sync/SyncFinished.h"
#include "core/sync/SyncQueued.h"
#include "core/sync/TrackMatching.h"
#include "core/sync/TrackStatusChanged.h"
#include "core/sync/TracksMerged.h"
#include "core/sync/TracksQueued.h"
#include "core/sync/TracksToDownload.h"
#include "core/sync/TracksToMatch.h"
#include "core/tools/ToolMissing.h"
#include "platform/Cancelled.h"

namespace westie::core {

namespace {

constexpr int MatchTries = 4;
// How many tracks a refused one lets go first, and how long it waits at least
constexpr std::size_t RetryGap = 10;
constexpr std::chrono::seconds RetryDelay{10};

std::string counted(std::size_t count, std::string_view thing) {
    return std::format("{} {}{}", count, thing, count == 1 ? "" : "s");
}

// Each track once, as the playlist's rows count them
std::size_t playlist_size(const State& state, const std::string& playlist) {
    auto found = state.playlists.find(playlist);
    return found == state.playlists.end() ? 0 : std::set<std::string>(found->second.sources.begin(), found->second.sources.end()).size();
}

}

SyncEngine::SyncEngine(
        const YtDlp& yt_dlp, ToolFolder& tools, platform::HttpClient& http, const platform::TextNormalizer& normalizer, SpotifyUrls spotify,
        YouTubeMusicSearch& youtube_music, Library& library, EventSink& events, Log& log, Downloads downloads
) :
        yt_dlp_(yt_dlp),
        tools_(tools),
        http_(http),
        spotify_(std::move(spotify)),
        normalizer_(normalizer),
        matcher_(youtube_music, normalizer, log),
        library_(library),
        events_(events),
        log_(log),
        downloads_(std::move(downloads)) {}

void SyncEngine::sync(const std::vector<std::string>& playlists) {
    std::unique_lock lock(mutex_);
    join_cancelled_run(lock);

    SyncQueued queued;
    for (const std::string& playlist : playlists) {
        if (current_ == playlist || std::ranges::contains(queue_, playlist)) {
            continue;
        }
        queue_.push_back(playlist);
        queued.playlists.push_back(playlist);
    }
    if (queued.playlists.empty()) {
        return;
    }
    std::string what = counted(queued.playlists.size(), "playlist");
    LogSubject subject = queued.playlists.size() == 1 ? playlist_subject(*library_.snapshot(), queued.playlists.front()) : LogSubject{};
    log_.write(LogLevel::Info, running_ ? std::format("Added {} to the update", what) : std::format("Update started: {}", what), std::move(subject));
    // Under the lock, so the run can't report on these playlists before the UI knows they're queued
    events_.emit(std::move(queued));
    // An update looks at every track that waits, also in a run that retry started
    only_.reset();
    if (running_) {
        wake_.notify_all();
        return;
    }
    start_run(lock);
}

void SyncEngine::retry(const std::vector<std::string>& tracks) {
    std::vector<std::string> retried;
    try {
        library_.update([&](State& state) { retried = apply_retry(state, tracks); });
    } catch (const std::exception& error) {
        log_.write(LogLevel::Error, std::format("Couldn't save the state: {}", error.what()));
        return;
    }
    if (retried.empty()) {
        return;
    }
    LogSubject subject = retried.size() == 1 ? track_subject(*library_.snapshot(), retried.front()) : LogSubject{};
    log_.write(LogLevel::Info, std::format("Retrying {}", counted(retried.size(), "track")), std::move(subject));

    std::unique_lock lock(mutex_);
    join_cancelled_run(lock);
    if (running_) {
        retried_.insert(retried_.end(), retried.begin(), retried.end());
        if (only_) {
            only_->insert(retried.begin(), retried.end());
        }
        wake_.notify_all();
        return;
    }
    only_.emplace(retried.begin(), retried.end());
    // Under the lock, as for a sync, so the UI knows of the run before it reports
    events_.emit(SyncQueued{});
    start_run(lock);
}

// A cancelled run would drop what comes now with its queue, so that waits for it to end and starts a run of its own
void SyncEngine::join_cancelled_run(std::unique_lock<std::mutex>& lock) {
    if (running_ && worker_.get_stop_token().stop_requested()) {
        lock.unlock();
        worker_.join();
        lock.lock();
    }
}

void SyncEngine::start_run(std::unique_lock<std::mutex>& lock) {
    running_ = true;
    retried_.clear();
    lock.unlock();
    // Assigning joins the run before, which has ended already
    worker_ = std::jthread([this](std::stop_token stop) { work(stop); });
}

void SyncEngine::cancel() noexcept {
    worker_.request_stop();
}

void SyncEngine::work(const std::stop_token& stop) {
    clear_work_folder();
    MatchQueue tracks;
    // Whether a fetch may have brought tracks to match since they were last looked for
    bool fetched = true;
    // Whether a fetch or a match may have made tracks ready to download since they were last looked for
    bool changed = true;
    std::optional<DownloadPool> downloads;
    while (true) {
        if (take_retried(tracks, downloads)) {
            fetched = true;
            changed = true;
        }
        // A playlist queued meanwhile goes before the next track
        if (std::optional<std::string> playlist = take_next(stop)) {
            fetch(*playlist, stop);
            fetched = true;
            changed = true;
            continue;
        }
        if (fetched && !stop.stop_requested()) {
            merge_library_matches();
            queue_tracks(tracks);
            fetched = false;
        }
        if (!tracks.waiting.empty() && !stop.stop_requested()) {
            if (std::optional<QueuedMatch> next = take_track(tracks, stop)) {
                match(std::move(*next), tracks, stop);
                changed = true;
            }
            continue;
        }
        if (changed && !stop.stop_requested()) {
            queue_downloads(downloads, stop);
            changed = false;
        }
        // The downloads go on while a playlist queued meanwhile is fetched and matched
        if (downloads && !stop.stop_requested() && !wait_for_downloads(*downloads, stop)) {
            continue;
        }
        // Joined before the run ends, so no download reports after it
        downloads.reset();
        if (finish(stop)) {
            return;
        }
    }
}

// The tracks retried during the run may have been tried in it already, so the queues forget them first
bool SyncEngine::take_retried(MatchQueue& queue, std::optional<DownloadPool>& pool) {
    std::vector<std::string> retried;
    {
        std::scoped_lock lock(mutex_);
        retried = std::exchange(retried_, {});
    }
    if (retried.empty()) {
        return false;
    }
    for (const std::string& track : retried) {
        queue.queued.erase(track);
    }
    if (pool) {
        pool->forget(retried);
    }
    return true;
}

std::vector<QueuedTrack> SyncEngine::in_run(std::vector<QueuedTrack> tracks) {
    std::scoped_lock lock(mutex_);
    if (only_) {
        std::erase_if(tracks, [this](const QueuedTrack& track) { return !only_->contains(track.track); });
    }
    return tracks;
}

void SyncEngine::queue_downloads(std::optional<DownloadPool>& pool, const std::stop_token& stop) {
    std::vector<QueuedTrack> ready = in_run(tracks_to_download(*library_.snapshot()));
    if (ready.empty()) {
        return;
    }
    if (!pool) {
        pool.emplace(
                DownloadServices{
                    .download = downloads_.download,
                    .tools = tools_,
                    .files = downloads_.files,
                    .settings = downloads_.settings,
                    .library = library_,
                    .events = events_,
                    .log = log_,
                },
                downloads_.work_folder, stop,
                [this] {
                    // Under the lock, so the run can't miss it between looking and waiting
                    { std::scoped_lock lock(mutex_); }
                    wake_.notify_all();
                },
                // Joins the run's queues as a retried track does, so it is matched again
                [this](const std::string& track) {
                    {
                        std::scoped_lock lock(mutex_);
                        retried_.push_back(track);
                    }
                    wake_.notify_all();
                }
        );
    }
    pool->add(std::move(ready));
}

// True once the downloads are done, or the run is stopped; false when a playlist was queued meanwhile
bool SyncEngine::wait_for_downloads(DownloadPool& pool, const std::stop_token& stop) {
    std::unique_lock lock(mutex_);
    wake_.wait(lock, stop, [&] { return !queue_.empty() || !retried_.empty() || pool.idle(); });
    return (queue_.empty() && retried_.empty()) || stop.stop_requested();
}

// What an earlier run left, as one that was stopped with the app
void SyncEngine::clear_work_folder() {
    try {
        downloads_.files.remove_tree(downloads_.work_folder);
    } catch (const std::exception& error) {
        log_.write(LogLevel::Debug, std::format("Couldn't clear the work folder: {}", error.what()));
    }
}

std::optional<std::string> SyncEngine::take_next(const std::stop_token& stop) {
    std::scoped_lock lock(mutex_);
    current_.reset();
    if (stop.stop_requested() || queue_.empty()) {
        return std::nullopt;
    }
    current_ = std::move(queue_.front());
    queue_.pop_front();
    return current_;
}

bool SyncEngine::finish(const std::stop_token& stop) {
    std::scoped_lock lock(mutex_);
    // A sync or a retry while the run matched its last track queued more
    if (!stop.stop_requested() && (!queue_.empty() || !retried_.empty())) {
        return false;
    }
    queue_.clear();
    retried_.clear();
    only_.reset();
    running_ = false;
    log_.write(LogLevel::Info, stop.stop_requested() ? "Update cancelled" : "Update finished");
    // Under the lock, so a sync that starts the next run comes after this in the events
    events_.emit(SyncFinished{.cancelled = stop.stop_requested()});
    return true;
}

void SyncEngine::fetch(const std::string& playlist, const std::stop_token& stop) {
    std::optional<PlaylistLink> link = playlist_link_of_key(playlist);
    if (!link) {
        return;
    }
    // An exception that leaves a thread ends the process, so it becomes the playlist's error instead
    try {
        PlaylistContents contents = fetch_contents(*link, stop);
        library_.update([&](State& state) { apply_fetch(state, playlist, std::move(contents), std::chrono::system_clock::now()); });
        std::shared_ptr<const State> state = library_.snapshot();
        log_.write(LogLevel::Info, std::format("Fetched {}", counted(playlist_size(*state, playlist), "track")), playlist_subject(*state, playlist));
        events_.emit(PlaylistFetched{.playlist = playlist});
    } catch (const platform::Cancelled&) {
        return;
    } catch (const std::exception& error) {
        PlaylistFetchFailed failed{.playlist = playlist, .error = error.what()};
        LogSubject subject = playlist_subject(*library_.snapshot(), playlist);
        log_.write(LogLevel::Error, std::format("Couldn't fetch the playlist: {}", one_line(failed.error)), subject);
        try {
            library_.update([&](State& state) { apply_fetch_failure(state, playlist, failed.error, std::chrono::system_clock::now()); });
        } catch (const std::exception& save_error) {
            // A state that can't be saved now keeps the playlist's previous error; the event still tells the UI
            log_.write(LogLevel::Error, std::format("Couldn't save the state: {}", save_error.what()), std::move(subject));
        }
        events_.emit(std::move(failed));
    }
}

PlaylistContents SyncEngine::fetch_contents(const PlaylistLink& link, const std::stop_token& stop) {
    log_.write(LogLevel::Debug, std::format("Fetching {}", link.url()), playlist_subject(*library_.snapshot(), link.key()));
    events_.emit(PlaylistFetching{.playlist = link.key()});
    switch (link.type) {
    case SourceType::YouTubeMusic: return fetch_when_installed(link, stop);
    case SourceType::Spotify:      return fetch_spotify_playlist(http_, spotify_, link, [&](const PlaylistContents& so_far) { show_page(link, so_far); }, stop);
    }
    return {};
}

void SyncEngine::show_page(const PlaylistLink& link, const PlaylistContents& so_far) {
    library_.update([&](State& state) { apply_fetch_page(state, link.key(), so_far, std::chrono::system_clock::now()); });
    std::shared_ptr<const State> state = library_.snapshot();
    log_.write(LogLevel::Debug, std::format("Fetched a page: {} so far", counted(playlist_size(*state, link.key()), "track")), playlist_subject(*state, link.key()));
    events_.emit(PlaylistPageFetched{.playlist = link.key()});
}

PlaylistContents SyncEngine::fetch_when_installed(const PlaylistLink& link, const std::stop_token& stop) {
    while (true) {
        std::uint64_t installs = tools_.installs(ToolPackage::YtDlp);
        try {
            return fetch_youtube_music_playlist(yt_dlp_, link, playlist_subject(*library_.snapshot(), link.key()), stop);
        } catch (const ToolMissing& missing) {
            log_.write(
                    LogLevel::Warning, std::format("Waiting for {} to install", tool_package_name(missing.package())),
                    playlist_subject(*library_.snapshot(), link.key())
            );
            events_.emit(PlaylistFetching{.playlist = link.key(), .waiting_for = {missing.package()}});
            if (!tools_.wait_for_install_after(missing.package(), installs, stop)) {
                throw platform::Cancelled();
            }
            events_.emit(PlaylistFetching{.playlist = link.key()});
        }
    }
}

void SyncEngine::merge_library_matches() {
    // An exception that leaves a thread ends the process; the library is looked at again next sync
    try {
        std::shared_ptr<const State> before = library_.snapshot();
        std::vector<LibraryMatch> matches = library_matches(*before, normalizer_);
        if (matches.empty()) {
            return;
        }
        std::vector<std::optional<std::string>> survivors;
        library_.update([&](State& state) {
            survivors.clear();
            for (const LibraryMatch& match : matches) {
                survivors.push_back(apply_match(state, match.track, match.match));
            }
        });
        // These merges have no events of their own, so the lines are about the tracks that are left
        for (std::size_t index = 0; index < matches.size(); ++index) {
            LogSubject subject = track_subject(*before, matches[index].track);
            if (survivors[index]) {
                subject.track = *survivors[index];
            }
            log_match(std::move(subject), matches[index].match, " from the library", survivors[index].has_value());
        }
    } catch (const std::exception& error) {
        log_.write(LogLevel::Error, std::format("Couldn't match tracks from the library: {}", error.what()));
    }
}

void SyncEngine::queue_tracks(MatchQueue& queue) {
    TracksQueued queued;
    for (QueuedTrack& track : in_run(tracks_to_match(*library_.snapshot()))) {
        if (queue.queued.insert(track.track).second) {
            queue.waiting.push_back({.track = track.track});
            queued.tracks.push_back(std::move(track));
        }
    }
    if (!queued.tracks.empty()) {
        log_.write(LogLevel::Debug, std::format("{} to match", counted(queued.tracks.size(), "track")));
        events_.emit(std::move(queued));
    }
}

std::optional<SyncEngine::QueuedMatch> SyncEngine::take_track(MatchQueue& queue, const std::stop_token& stop) {
    std::chrono::steady_clock::time_point now = std::chrono::steady_clock::now();
    auto due = std::ranges::find_if(queue.waiting, [&](const QueuedMatch& queued) { return queued.not_before <= now; });
    if (due != queue.waiting.end()) {
        QueuedMatch next = std::move(*due);
        queue.waiting.erase(due);
        return next;
    }
    std::chrono::steady_clock::time_point first = std::ranges::min(queue.waiting | std::views::transform(&QueuedMatch::not_before));
    std::unique_lock lock(mutex_);
    // A playlist queued meanwhile ends the wait, so it is fetched first, and so do tracks retried meanwhile
    wake_.wait_until(lock, stop, first, [this] { return !queue_.empty() || !retried_.empty(); });
    return std::nullopt;
}

void SyncEngine::match(QueuedMatch queued, MatchQueue& queue, const std::stop_token& stop) {
    std::shared_ptr<const State> state = library_.snapshot();
    auto found = state->tracks.find(queued.track);
    // Merged or removed with its playlist since it was queued; the event still ends it for the UI's counts
    if (found == state->tracks.end() || !needs_match(*state, found->second)) {
        events_.emit(TrackStatusChanged{.track = queued.track});
        return;
    }
    events_.emit(TrackMatching{.track = queued.track});
    const Source& source = state->sources.at(*source_to_match(*state, found->second));
    LogSubject subject = track_subject(*state, queued.track);
    int tries = queued.tries + 1;

    // An exception that leaves a thread ends the process, so it becomes the track's failure instead
    try {
        std::expected<Match, std::string> matched = matcher_.match(source, track_ytm_ids(*state), state->unavailable_ytm_ids, subject, stop);
        if (!matched) {
            fail_match(queued.track, TrackFailure::NoMatch, std::move(matched.error()), tries);
            return;
        }
        std::optional<std::string> survivor;
        library_.update([&](State& changed) { survivor = apply_match(changed, queued.track, *matched); });
        // About the matched track even when it merged: the UI moves its lines to the survivor with TracksMerged
        log_match(std::move(subject), *matched, "", survivor.has_value());
        if (survivor) {
            events_.emit(TracksMerged{.merged = queued.track, .survivor = std::move(*survivor)});
        } else {
            events_.emit(TrackStatusChanged{.track = queued.track});
        }
    } catch (const platform::Cancelled&) {
        return;
    } catch (const std::exception& error) {
        if (!refused_search(error) || tries >= MatchTries) {
            fail_match(queued.track, TrackFailure::Search, error.what(), tries);
            return;
        }
        // YouTube Music refuses a search now and then while the ones around it go through, so the tracks after it go first
        queued.tries = tries;
        queued.not_before = std::chrono::steady_clock::now() + RetryDelay;
        queue.waiting.insert(queue.waiting.begin() + static_cast<std::ptrdiff_t>(std::min(RetryGap, queue.waiting.size())), std::move(queued));
    }
}

void SyncEngine::fail_match(const std::string& track, TrackFailure failure, std::string error, int tries) {
    LogSubject subject = track_subject(*library_.snapshot(), track);
    std::string retried = tries > 1 ? std::format(" (tried {} times)", tries) : "";
    log_.write(
            LogLevel::Error,
            failure == TrackFailure::NoMatch ? std::string("No match found") : std::format("Couldn't search YouTube Music: {}{}", one_line(error), retried),
            subject
    );
    TrackStatusChanged changed{.track = track, .status = TrackStatus::Failed, .failure = failure, .error = std::move(error)};
    try {
        library_.update([&](State& state) { apply_match_failure(state, track, changed.failure, changed.error); });
    } catch (const std::exception& save_error) {
        // A state that can't be saved keeps the track as it was; the event still tells the UI
        log_.write(LogLevel::Error, std::format("Couldn't save the state: {}", save_error.what()), std::move(subject));
    }
    events_.emit(std::move(changed));
}

void SyncEngine::log_match(LogSubject subject, const Match& match, std::string_view from, bool merged) {
    if (needs_review(match)) {
        std::string doubts = doubts_text(match.doubts);
        log_.write(LogLevel::Warning, std::format("Needs review: {}{}{}{}", match_text(match), from, doubts.empty() ? "" : ", ", doubts), std::move(subject));
        return;
    }
    log_.write(
            LogLevel::Info, std::format("Matched {}{}{}", match_text(match), from, merged ? ", and merged into the track with that YouTube Music id" : ""),
            std::move(subject)
    );
}

}
