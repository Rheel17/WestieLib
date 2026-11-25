#pragma once

#include <map>
#include <optional>
#include <span>
#include <string>
#include <vector>

#include "core/sync/DownloadEvents.h"
#include "core/sync/PlaylistFetching.h"
#include "core/sync/SyncQueued.h"
#include "core/sync/TrackMatching.h"
#include "core/sync/TracksQueued.h"
#include "core/tools/ToolPackage.h"
#include "ui/sync/TrackStage.h"
#include "ui/tracks/TrackActivity.h"

namespace westie::ui {

class SyncBoard {
public:
    void apply(const core::SyncQueued& queued);
    void apply(const core::PlaylistFetching& fetching);
    void apply(const core::TracksQueued& queued);
    void apply(const core::TrackMatching& matching);
    void apply(const core::DownloadsQueued& queued);
    void apply(const core::TrackDownloading& downloading);
    void apply(const core::TrackProgress& progress);
    void apply(const core::DownloadsWaiting& waiting);
    void finish_playlist(const std::string& playlist);
    void finish_track(const std::string& track);
    void finish();

    bool running() const noexcept;
    bool queued(const std::string& playlist) const;
    // The playlist being synced
    const std::optional<std::string>& current() const noexcept;
    // What the sync of the current playlist waits for
    std::span<const core::ToolPackage> fetch_waiting_for() const noexcept;
    // What all that's left of the run waits for
    std::span<const core::ToolPackage> waiting_for() const noexcept;
    // What the run's downloads wait for
    std::span<const core::ToolPackage> downloads_waiting_for() const noexcept;
    // Whether the run is past its syncs, matching tracks
    bool matching() const noexcept;
    // Whether it is past its matches too, downloading tracks
    bool downloading() const noexcept;
    // Of the phase it is in: what has ended, what goes on right now, and all it was given
    int finished() const noexcept;
    int under_way() const noexcept;
    int total() const noexcept;
    // While the run has tracks of the playlist left to match
    std::optional<StageProgress> match_progress(const std::string& playlist) const;
    bool matching_now(const std::string& playlist) const;
    // While it has tracks of the playlist left to download
    std::optional<StageProgress> download_progress(const std::string& playlist) const;
    int downloading_in(const std::string& playlist) const;
    TrackActivities activities() const;

private:
    bool running_ = false;
    // Playlists the run has done, and all it was given
    int finished_ = 0;
    int total_ = 0;
    std::vector<std::string> queue_;
    std::optional<std::string> current_;
    std::vector<core::ToolPackage> fetch_waiting_for_;
    TrackStage matches_;
    TrackStage downloads_;
    std::map<std::string, int> percents_;
    std::vector<core::ToolPackage> downloads_waiting_for_;
};

}
