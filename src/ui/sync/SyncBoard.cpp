#include "ui/sync/SyncBoard.h"

#include <algorithm>

namespace westie::ui {

void SyncBoard::apply(const core::SyncQueued& queued) {
    running_ = true;
    queue_.insert(queue_.end(), queued.playlists.begin(), queued.playlists.end());
    total_ += static_cast<int>(queued.playlists.size());
}

void SyncBoard::apply(const core::PlaylistFetching& fetching) {
    std::erase(queue_, fetching.playlist);
    current_ = fetching.playlist;
    waiting_for_ = fetching.waiting_for;
}

void SyncBoard::apply(const core::TracksQueued& queued) {
    tracks_total_ += static_cast<int>(queued.tracks.size());
    for (const core::QueuedTrack& track : queued.tracks) {
        for (const std::string& playlist : track.playlists) {
            ++playlist_matches_[playlist].queued;
        }
        track_playlists_[track.track] = track.playlists;
    }
}

void SyncBoard::apply(const core::TrackMatching& matching) {
    if (auto found = track_playlists_.find(matching.track); found != track_playlists_.end()) {
        matching_playlists_ = found->second;
    }
}

void SyncBoard::finish_playlist(const std::string& playlist) {
    if (current_ == playlist) {
        current_.reset();
        waiting_for_.clear();
    }
    ++finished_;
}

void SyncBoard::finish_track(const std::string& track) {
    auto found = track_playlists_.find(track);
    // Not one the run queued
    if (found == track_playlists_.end()) {
        return;
    }
    for (const std::string& playlist : found->second) {
        ++playlist_matches_[playlist].ended;
    }
    track_playlists_.erase(found);
    ++tracks_finished_;
}

void SyncBoard::finish() {
    *this = SyncBoard();
}

bool SyncBoard::running() const noexcept {
    return running_;
}

bool SyncBoard::queued(const std::string& playlist) const {
    return std::ranges::contains(queue_, playlist);
}

const std::optional<std::string>& SyncBoard::current() const noexcept {
    return current_;
}

std::span<const core::ToolPackage> SyncBoard::waiting_for() const noexcept {
    return waiting_for_;
}

bool SyncBoard::matching() const noexcept {
    return tracks_total_ > 0 && !current_ && queue_.empty();
}

int SyncBoard::finished() const noexcept {
    return matching() ? tracks_finished_ : finished_;
}

int SyncBoard::total() const noexcept {
    return matching() ? tracks_total_ : total_;
}

std::optional<MatchProgress> SyncBoard::match_progress(const std::string& playlist) const {
    auto found = playlist_matches_.find(playlist);
    if (found == playlist_matches_.end() || found->second.ended >= found->second.queued) {
        return std::nullopt;
    }
    return found->second;
}

bool SyncBoard::matching_now(const std::string& playlist) const {
    return std::ranges::contains(matching_playlists_, playlist);
}

}
