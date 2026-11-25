#pragma once

#include <map>
#include <optional>
#include <set>
#include <string>
#include <vector>

#include "core/sync/TracksQueued.h"

namespace westie::ui {

struct StageProgress {
    int ended = 0;
    int queued = 0;
};

// The tracks an update queued for one of its steps, matching or downloading, counted for each playlist that lists them
class TrackStage {
public:
    void queue(const std::vector<core::QueuedTrack>& tracks);
    void start(const std::string& track);
    // False for a track this step didn't queue, or that ended already
    bool finish(const std::string& track);

    bool has(const std::string& track) const;
    bool started(const std::string& track) const;
    int finished() const noexcept;
    int total() const noexcept;
    bool done() const noexcept;
    // While the step has tracks of the playlist left
    std::optional<StageProgress> progress(const std::string& playlist) const;
    bool last_started_in(const std::string& playlist) const;
    int started_in(const std::string& playlist) const;
    int started_count() const noexcept;
    std::vector<std::string> waiting() const;

private:
    int finished_ = 0;
    int total_ = 0;
    std::map<std::string, StageProgress> playlists_;
    // The queued tracks that haven't ended, with the playlists that list them
    std::map<std::string, std::vector<std::string>> tracks_;
    std::set<std::string> started_;
    // The playlists of the track started last, kept between tracks so their rows keep spinning
    std::vector<std::string> last_playlists_;
};

}
