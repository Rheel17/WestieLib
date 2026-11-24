#include <chrono>
#include <format>
#include <string>
#include <utility>

#include "Checks.h"
#include "UnitChecks.h"
#include "core/state/State.h"
#include "core/sync/TracksToDownload.h"

namespace westie::tests {

namespace {

void add(core::State& state, const std::string& key, core::Track track, core::Source source = {}) {
    source.track = key;
    track.sources.insert(track.sources.begin(), key);
    state.sources[key] = std::move(source);
    state.tracks[key] = std::move(track);
}

core::Source matched(double confidence, bool confirmed = false) {
    return {.match = core::Match{.ytm_id = "matchedId01", .confidence = confidence, .confirmed = confirmed}};
}

std::string queue_text(const core::State& state) {
    std::string text;
    for (const core::QueuedTrack& queued : core::tracks_to_download(state)) {
        text += std::format("{}{} [{}]", text.empty() ? "" : " / ", queued.track, queued.playlists.size());
    }
    return text;
}

}

void check_downloads_to_queue() {
    core::State state;
    add(state, "ytm:pending0001", {});
    add(state, "ytm:downloaded1", {.status = core::TrackStatus::Downloaded});
    add(state, "ytm:failedDown1", {.status = core::TrackStatus::Failed, .failure = core::TrackFailure::Download});
    add(state, "spotify:unmatched", {});
    add(state, "spotify:accepted", {}, matched(1.0));
    add(state, "spotify:review", {}, matched(0.75));
    add(state, "spotify:confirmed", {}, matched(0.75, true));
    add(state, "spotify:overridden", {.ytm_override = "overridden1"}, matched(0.75));
    add(state, "spotify:noMatch", {.status = core::TrackStatus::Failed, .failure = core::TrackFailure::NoMatch});
    add(state, "ytm:notListed01", {});

    std::chrono::system_clock::time_point day{};
    state.playlists["spotify:second"] = {
        .added_at = day + std::chrono::hours(24),
        .sources = {"spotify:unmatched", "spotify:accepted", "spotify:review", "spotify:confirmed", "spotify:overridden", "spotify:noMatch", "ytm:pending0001"},
    };
    state.playlists["ytm:first"] = {.added_at = day, .sources = {"ytm:failedDown1", "ytm:downloaded1", "ytm:pending0001"}};

    std::string actual = queue_text(state);
    std::string expected = "ytm:failedDown1 [1] / ytm:pending0001 [2] / spotify:accepted [1] / spotify:confirmed [1] / spotify:overridden [1]";
    check(actual == expected, std::format("tracks to download: {}, expected {}", actual, expected));
}

}
