#include "ui/sync/SyncBoard.h"

#include <algorithm>

namespace westie::ui {

void SyncBoard::apply(const core::SyncQueued& queued) {
    running_ = true;
    queue_.insert(queue_.end(), queued.playlists.begin(), queued.playlists.end());
}

void SyncBoard::apply(const core::PlaylistFetching& fetching) {
    std::erase(queue_, fetching.playlist);
    current_ = fetching.playlist;
    waiting_for_ = fetching.waiting_for;
}

void SyncBoard::finish_playlist(const std::string& playlist) {
    if (current_ == playlist) {
        current_.reset();
        waiting_for_.clear();
    }
}

void SyncBoard::finish() {
    running_ = false;
    queue_.clear();
    current_.reset();
    waiting_for_.clear();
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

}
