#include "ui/sync/TrackStage.h"

#include <algorithm>
#include <ranges>

namespace westie::ui {

void TrackStage::queue(const std::vector<core::QueuedTrack>& tracks) {
    for (const core::QueuedTrack& track : tracks) {
        // Queued again, as a retried track is: it ends once more
        if (tracks_.contains(track.track)) {
            continue;
        }
        ++total_;
        for (const std::string& playlist : track.playlists) {
            ++playlists_[playlist].queued;
        }
        tracks_[track.track] = track.playlists;
    }
}

void TrackStage::start(const std::string& track) {
    if (auto found = tracks_.find(track); found != tracks_.end()) {
        last_playlists_ = found->second;
        started_.insert(track);
    }
}

bool TrackStage::finish(const std::string& track) {
    auto found = tracks_.find(track);
    if (found == tracks_.end()) {
        return false;
    }
    for (const std::string& playlist : found->second) {
        ++playlists_[playlist].ended;
    }
    tracks_.erase(found);
    started_.erase(track);
    ++finished_;
    return true;
}

bool TrackStage::has(const std::string& track) const {
    return tracks_.contains(track);
}

bool TrackStage::started(const std::string& track) const {
    return started_.contains(track);
}

int TrackStage::finished() const noexcept {
    return finished_;
}

int TrackStage::total() const noexcept {
    return total_;
}

bool TrackStage::done() const noexcept {
    return finished_ >= total_;
}

std::optional<StageProgress> TrackStage::progress(const std::string& playlist) const {
    auto found = playlists_.find(playlist);
    if (found == playlists_.end() || found->second.ended >= found->second.queued) {
        return std::nullopt;
    }
    return found->second;
}

bool TrackStage::last_started_in(const std::string& playlist) const {
    return std::ranges::contains(last_playlists_, playlist);
}

int TrackStage::started_in(const std::string& playlist) const {
    return static_cast<int>(std::ranges::count_if(started_, [&](const std::string& track) { return std::ranges::contains(tracks_.at(track), playlist); }));
}

int TrackStage::started_count() const noexcept {
    return static_cast<int>(started_.size());
}

std::vector<std::string> TrackStage::waiting() const {
    std::vector<std::string> waiting;
    for (const std::string& track : tracks_ | std::views::keys) {
        if (!started_.contains(track)) {
            waiting.push_back(track);
        }
    }
    return waiting;
}

}
