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
}

void SyncBoard::finish_playlist(const std::string& playlist) {
    if (current_ == playlist) {
        current_.reset();
        waiting_for_.clear();
    }
    ++finished_;
}

void SyncBoard::finish_track() {
    ++tracks_finished_;
}

void SyncBoard::finish() {
    running_ = false;
    queue_.clear();
    current_.reset();
    waiting_for_.clear();
    finished_ = 0;
    total_ = 0;
    tracks_finished_ = 0;
    tracks_total_ = 0;
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

}
