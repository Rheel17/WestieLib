#pragma once

#include <map>
#include <optional>
#include <span>
#include <string>
#include <vector>

#include "core/sync/PlaylistFetching.h"
#include "core/sync/SyncQueued.h"
#include "core/sync/TrackMatching.h"
#include "core/sync/TracksQueued.h"
#include "core/tools/ToolPackage.h"

namespace westie::ui {

struct MatchProgress {
    int ended = 0;
    int queued = 0;
};

class SyncBoard {
public:
    void apply(const core::SyncQueued& queued);
    void apply(const core::PlaylistFetching& fetching);
    void apply(const core::TracksQueued& queued);
    void apply(const core::TrackMatching& matching);
    void finish_playlist(const std::string& playlist);
    void finish_track(const std::string& track);
    void finish();

    bool running() const noexcept;
    bool queued(const std::string& playlist) const;
    // The playlist being synced
    const std::optional<std::string>& current() const noexcept;
    std::span<const core::ToolPackage> waiting_for() const noexcept;
    // Whether the run is past its syncs, matching tracks
    bool matching() const noexcept;
    int finished() const noexcept;
    int total() const noexcept;
    // While the run has tracks of the playlist left to match
    std::optional<MatchProgress> match_progress(const std::string& playlist) const;
    bool matching_now(const std::string& playlist) const;

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
    std::map<std::string, MatchProgress> playlist_matches_;
    // The queued tracks whose match hasn't ended, with the playlists that list them
    std::map<std::string, std::vector<std::string>> track_playlists_;
    std::optional<std::string> matching_track_;
};

}
