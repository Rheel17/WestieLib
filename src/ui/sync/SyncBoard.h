#pragma once

#include <optional>
#include <span>
#include <string>
#include <vector>

#include "core/sync/PlaylistFetching.h"
#include "core/sync/SyncQueued.h"
#include "core/sync/TracksQueued.h"
#include "core/tools/ToolPackage.h"

namespace westie::ui {

class SyncBoard {
public:
    void apply(const core::SyncQueued& queued);
    void apply(const core::PlaylistFetching& fetching);
    void apply(const core::TracksQueued& queued);
    void finish_playlist(const std::string& playlist);
    void finish_track();
    void finish();

    bool running() const noexcept;
    bool queued(const std::string& playlist) const;
    const std::optional<std::string>& current() const noexcept;
    std::span<const core::ToolPackage> waiting_for() const noexcept;
    bool matching() const noexcept;
    int finished() const noexcept;
    int total() const noexcept;

private:
    bool running_ = false;
    // Playlists the run has done, and all it was given
    int finished_ = 0;
    int total_ = 0;
    // The same for the tracks it matches
    int tracks_finished_ = 0;
    int tracks_total_ = 0;
    std::vector<std::string> queue_;
    std::optional<std::string> current_;
    std::vector<core::ToolPackage> waiting_for_;
};

}
