#include "core/sync/SyncEngine.h"

#include <algorithm>
#include <chrono>
#include <cstddef>
#include <cstdint>
#include <exception>
#include <expected>
#include <format>
#include <memory>
#include <set>
#include <utility>

#include "core/log/LogLevel.h"
#include "core/matching/LibraryMatches.h"
#include "core/matching/MatchText.h"
#include "core/sources/SourceType.h"
#include "core/sources/SpotifyPlaylist.h"
#include "core/sources/YouTubeMusicPlaylist.h"
#include "core/state/MatchReview.h"
#include "core/state/TrackVideos.h"
#include "core/sync/ApplyFetch.h"
#include "core/sync/ApplyMatch.h"
#include "core/sync/PlaylistFetchFailed.h"
#include "core/sync/PlaylistFetched.h"
#include "core/sync/PlaylistFetching.h"
#include "core/sync/PlaylistPageFetched.h"
#include "core/sync/SyncFinished.h"
#include "core/sync/SyncQueued.h"
#include "core/sync/TrackStatusChanged.h"
#include "core/sync/TracksMerged.h"
#include "core/sync/TracksQueued.h"
#include "core/sync/TracksToMatch.h"
#include "core/tools/ToolMissing.h"
#include "platform/Cancelled.h"

namespace westie::core {

namespace {

std::string counted(std::size_t count, std::string_view thing) {
    return std::format("{} {}{}", count, thing, count == 1 ? "" : "s");
}

// Each track once, as the playlist's rows count them
std::size_t playlist_size(const State& state, const std::string& playlist) {
    auto found = state.playlists.find(playlist);
    return found == state.playlists.end() ? 0 : std::set<std::string>(found->second.sources.begin(), found->second.sources.end()).size();
}

// A log line is a row in the pane
std::string one_line(std::string_view text) {
    std::string line;
    for (char character : text) {
        if (character == '\n') {
            line += "; ";
        } else if (character != '\r') {
            line += character;
        }
    }
    return line;
}

}

SyncEngine::SyncEngine(
        const YtDlp& yt_dlp, ToolFolder& tools, platform::HttpClient& http, const platform::TextNormalizer& normalizer, SpotifyUrls spotify,
        std::string youtube_music_api, StateStore& state, EventSink& events, Log& log
) :
        yt_dlp_(yt_dlp),
        tools_(tools),
        http_(http),
        spotify_(std::move(spotify)),
        normalizer_(normalizer),
        matcher_(http, normalizer, std::move(youtube_music_api)),
        state_(state),
        events_(events),
        log_(log) {}

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
    LogSubject subject = queued.playlists.size() == 1 ? playlist_subject(*state_.snapshot(), queued.playlists.front()) : LogSubject{};
    log_.write(LogLevel::Info, running_ ? std::format("Added {} to the sync", what) : std::format("Sync started: {}", what), std::move(subject));
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
    MatchQueue tracks;
    // Whether a fetch may have brought tracks to match since they were last looked for
    bool fetched = true;
    while (true) {
        // A playlist queued meanwhile goes before the next track
        if (std::optional<std::string> playlist = take_next(stop)) {
            fetch(*playlist, stop);
            fetched = true;
            continue;
        }
        if (fetched && !stop.stop_requested()) {
            merge_library_matches();
            queue_tracks(tracks);
            fetched = false;
        }
        if (!tracks.waiting.empty() && !stop.stop_requested()) {
            std::string track = std::move(tracks.waiting.front());
            tracks.waiting.pop_front();
            match(track, stop);
            continue;
        }
        if (finish(stop)) {
            return;
        }
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
    // A sync while the run matched its last track queued more playlists
    if (!stop.stop_requested() && !queue_.empty()) {
        return false;
    }
    queue_.clear();
    running_ = false;
    log_.write(LogLevel::Info, stop.stop_requested() ? "Sync cancelled" : "Sync finished");
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
        state_.update([&](State& state) { apply_fetch(state, playlist, std::move(contents), std::chrono::system_clock::now()); });
        std::shared_ptr<const State> state = state_.snapshot();
        log_.write(LogLevel::Info, std::format("Fetched {}", counted(playlist_size(*state, playlist), "track")), playlist_subject(*state, playlist));
        events_.emit(PlaylistFetched{.playlist = playlist});
    } catch (const platform::Cancelled&) {
        return;
    } catch (const std::exception& error) {
        PlaylistFetchFailed failed{.playlist = playlist, .error = error.what()};
        LogSubject subject = playlist_subject(*state_.snapshot(), playlist);
        log_.write(LogLevel::Error, std::format("Couldn't fetch the playlist: {}", one_line(failed.error)), subject);
        try {
            state_.update([&](State& state) { apply_fetch_failure(state, playlist, failed.error, std::chrono::system_clock::now()); });
        } catch (const std::exception& save_error) {
            // A state that can't be saved now keeps the playlist's previous error; the event still tells the UI
            log_.write(LogLevel::Error, std::format("Couldn't save the state: {}", save_error.what()), std::move(subject));
        }
        events_.emit(std::move(failed));
    }
}

PlaylistContents SyncEngine::fetch_contents(const PlaylistLink& link, const std::stop_token& stop) {
    log_.write(LogLevel::Debug, std::format("Fetching {}", link.url()), playlist_subject(*state_.snapshot(), link.key()));
    events_.emit(PlaylistFetching{.playlist = link.key()});
    switch (link.type) {
    case SourceType::YouTubeMusic: return fetch_when_installed(link, stop);
    case SourceType::Spotify:      return fetch_spotify_playlist(http_, spotify_, link, [&](const PlaylistContents& so_far) { show_page(link, so_far); }, stop);
    }
    return {};
}

void SyncEngine::show_page(const PlaylistLink& link, const PlaylistContents& so_far) {
    state_.update([&](State& state) { apply_fetch_page(state, link.key(), so_far, std::chrono::system_clock::now()); });
    std::shared_ptr<const State> state = state_.snapshot();
    log_.write(LogLevel::Debug, std::format("Fetched a page: {} so far", counted(playlist_size(*state, link.key()), "track")), playlist_subject(*state, link.key()));
    events_.emit(PlaylistPageFetched{.playlist = link.key()});
}

PlaylistContents SyncEngine::fetch_when_installed(const PlaylistLink& link, const std::stop_token& stop) {
    while (true) {
        std::uint64_t installs = tools_.installs(ToolPackage::YtDlp);
        try {
            return fetch_youtube_music_playlist(yt_dlp_, link, stop);
        } catch (const ToolMissing& missing) {
            log_.write(
                    LogLevel::Warning, std::format("Waiting for {} to install", tool_package_name(missing.package())),
                    playlist_subject(*state_.snapshot(), link.key())
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
        std::shared_ptr<const State> before = state_.snapshot();
        std::vector<LibraryMatch> matches = library_matches(*before, normalizer_);
        if (matches.empty()) {
            return;
        }
        std::vector<std::optional<std::string>> survivors;
        state_.update([&](State& state) {
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
    for (std::string& track : tracks_to_match(*state_.snapshot())) {
        if (queue.queued.insert(track).second) {
            queue.waiting.push_back(track);
            queued.tracks.push_back(std::move(track));
        }
    }
    if (!queued.tracks.empty()) {
        log_.write(LogLevel::Debug, std::format("{} to match", counted(queued.tracks.size(), "track")));
        events_.emit(std::move(queued));
    }
}

void SyncEngine::match(const std::string& track, const std::stop_token& stop) {
    std::shared_ptr<const State> state = state_.snapshot();
    auto found = state->tracks.find(track);
    // Merged or removed with its playlist since it was queued
    if (found == state->tracks.end() || !needs_match(*state, found->second)) {
        return;
    }
    const Source& source = state->sources.at(*first_source_of(found->second, SourceType::Spotify));

    // An exception that leaves a thread ends the process, so it becomes the track's failure instead
    try {
        std::expected<Match, std::string> matched = matcher_.match(source, track_videos(*state), stop);
        if (!matched) {
            fail_match(track, TrackFailure::NoMatch, std::move(matched.error()));
            return;
        }
        std::optional<std::string> survivor;
        state_.update([&](State& changed) { survivor = apply_match(changed, track, *matched); });
        // About the matched track even when it merged: the UI moves its lines to the survivor with TracksMerged
        log_match(track_subject(*state, track), *matched, "", survivor.has_value());
        if (survivor) {
            events_.emit(TracksMerged{.merged = track, .survivor = std::move(*survivor)});
        } else {
            events_.emit(TrackStatusChanged{.track = track});
        }
    } catch (const platform::Cancelled&) {
        return;
    } catch (const std::exception& error) {
        fail_match(track, TrackFailure::Search, error.what());
    }
}

void SyncEngine::fail_match(const std::string& track, TrackFailure failure, std::string error) {
    LogSubject subject = track_subject(*state_.snapshot(), track);
    log_.write(
            LogLevel::Error,
            failure == TrackFailure::NoMatch ? std::format("No match found. {}", one_line(error)) : std::format("Couldn't match: {}", one_line(error)),
            subject
    );
    TrackStatusChanged changed{.track = track, .status = TrackStatus::Failed, .failure = failure, .error = std::move(error)};
    try {
        state_.update([&](State& state) { apply_match_failure(state, track, changed.failure, changed.error); });
    } catch (const std::exception& save_error) {
        // A state that can't be saved keeps the track as it was; the event still tells the UI
        log_.write(LogLevel::Error, std::format("Couldn't save the state: {}", save_error.what()), std::move(subject));
    }
    events_.emit(std::move(changed));
}

void SyncEngine::log_match(LogSubject subject, const Match& match, std::string_view from, bool merged) {
    if (needs_review(match)) {
        std::string doubts = doubts_text(match.doubts);
        log_.write(LogLevel::Warning, std::format("Needs review: {}{}{}{}", match_text(match), from, doubts.empty() ? "" : ". ", doubts), std::move(subject));
        return;
    }
    log_.write(
            LogLevel::Info, std::format("Matched {}{}{}", match_text(match), from, merged ? ", and merged into the track that has that video" : ""),
            std::move(subject)
    );
}

}
