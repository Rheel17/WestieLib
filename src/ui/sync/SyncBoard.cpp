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
    fetch_waiting_for_ = fetching.waiting_for;
}

void SyncBoard::apply(const core::TracksQueued& queued) {
    matches_.queue(queued.tracks);
}

void SyncBoard::apply(const core::TrackMatching& matching) {
    matches_.start(matching.track);
}

void SyncBoard::apply(const core::DownloadsQueued& queued) {
    downloads_.queue(queued.tracks);
}

void SyncBoard::apply(const core::TrackDownloading& downloading) {
    if (downloads_.has(downloading.track)) {
        downloads_.start(downloading.track);
        percents_[downloading.track] = 0;
    }
}

void SyncBoard::apply(const core::TrackProgress& progress) {
    if (downloads_.started(progress.track)) {
        percents_[progress.track] = progress.percent;
    }
}

void SyncBoard::apply(const core::DownloadsWaiting& waiting) {
    downloads_waiting_for_ = waiting.waiting_for;
}

void SyncBoard::finish_playlist(const std::string& playlist) {
    if (current_ == playlist) {
        current_.reset();
        fetch_waiting_for_.clear();
    }
    ++finished_;
}

// A track is in one step at a time, and an event that ends it ends that one
void SyncBoard::finish_track(const std::string& track) {
    if (!matches_.finish(track)) {
        downloads_.finish(track);
        if (auto percent = percents_.find(track); percent != percents_.end()) {
            ended_[track] = percent->second;
            percents_.erase(percent);
        }
    }
}

void SyncBoard::rows_requested() {
    for (const auto& [track, percent] : ended_) {
        ended_in_rows_[track] = percent;
    }
    ended_.clear();
}

void SyncBoard::rows_shown() {
    ended_in_rows_.clear();
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

std::span<const core::ToolPackage> SyncBoard::fetch_waiting_for() const noexcept {
    return fetch_waiting_for_;
}

std::span<const core::ToolPackage> SyncBoard::waiting_for() const noexcept {
    if (current_) {
        return fetch_waiting_for_;
    }
    return downloading() ? std::span<const core::ToolPackage>(downloads_waiting_for_) : std::span<const core::ToolPackage>();
}

std::span<const core::ToolPackage> SyncBoard::downloads_waiting_for() const noexcept {
    return downloads_waiting_for_;
}

bool SyncBoard::matching() const noexcept {
    return matches_.total() > 0 && !current_ && queue_.empty() && (!matches_.done() || downloads_.total() == 0);
}

bool SyncBoard::downloading() const noexcept {
    return downloads_.total() > 0 && !current_ && queue_.empty() && matches_.done();
}

int SyncBoard::finished() const noexcept {
    if (downloading()) {
        return downloads_.finished();
    }
    return matching() ? matches_.finished() : finished_;
}

// The playlist being synced or the track being matched, or the tracks downloading side by side
int SyncBoard::under_way() const noexcept {
    return downloading() ? downloads_.started_count() : 1;
}

int SyncBoard::total() const noexcept {
    if (downloading()) {
        return downloads_.total();
    }
    return matching() ? matches_.total() : total_;
}

std::optional<StageProgress> SyncBoard::match_progress(const std::string& playlist) const {
    return matches_.progress(playlist);
}

bool SyncBoard::matching_now(const std::string& playlist) const {
    return matches_.last_started_in(playlist);
}

std::optional<StageProgress> SyncBoard::download_progress(const std::string& playlist) const {
    return downloads_.progress(playlist);
}

int SyncBoard::downloading_in(const std::string& playlist) const {
    return downloads_.started_in(playlist);
}

TrackActivities SyncBoard::activities() const {
    TrackActivities activities;
    // A track downloaded again shows its new download
    for (const std::map<std::string, int>* percents : {&ended_in_rows_, &ended_, &percents_}) {
        for (const auto& [track, percent] : *percents) {
            activities[track].percent = percent;
        }
    }
    if (!downloads_waiting_for_.empty()) {
        for (const std::string& track : downloads_.waiting()) {
            activities[track].waiting_for = downloads_waiting_for_;
        }
    }
    return activities;
}

}
