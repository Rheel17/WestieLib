#include <format>
#include <optional>
#include <source_location>
#include <string>
#include <string_view>
#include <utility>

#include "Checks.h"
#include "UnitChecks.h"
#include "core/download/UnavailableError.h"
#include "core/state/MatchReview.h"
#include "core/state/State.h"
#include "core/state/TrackYtmIds.h"
#include "core/sync/ApplyDownload.h"
#include "core/sync/TracksToDownload.h"
#include "core/sync/TracksToMatch.h"

namespace westie::tests {

namespace {

constexpr std::string_view DeadId = "deadDeadId1";
constexpr std::string_view LiveId = "liveLiveId1";

void check_error(std::string_view error, bool expected, std::source_location where = std::source_location::current()) {
    bool actual = core::is_unavailable_error(error);
    check(actual == expected, std::format("\"{}\": {}, expected {}", error, actual, expected), where);
}

void check_errors() {
    check_error("ERROR: [youtube] 1YKAZmwRGj8: Video unavailable. This video is not available", true);
    check_error("ERROR: [youtube] abc: The uploader has not made this video available in your country", true);
    check_error("ERROR: [youtube] abc: This video has been removed by the uploader", true);
    check_error("ERROR: [youtube] wLCp11IhlFY: This video is only available to Music Premium members", true);
    check_error("[youtube] abc: Downloading webpage\nERROR: [youtube] abc: Video unavailable. This video is not available", true);
    check_error("ERROR: [youtube] 5h9rVwb9lC0: Sign in to confirm your age. This video may be inappropriate for some users.", false);
    check_error("ERROR: [youtube] abc: Sign in to confirm you’re not a bot", false);
    check_error("ERROR: [youtube] abc: Unable to download API page: [SSL: UNEXPECTED_EOF_WHILE_READING]", false);
    // Only yt-dlp's errors count, not a title it printed
    check_error("[download] Video unavailable - Live.opus", false);
    check_error("yt-dlp exited with code 1", false);
}

std::string ytm_key(std::string_view id) {
    return std::format("ytm:{}", id);
}

core::Match match_to(std::string_view id, double confidence) {
    return {.ytm_id = std::string(id), .confidence = confidence, .title = "Song", .artists = {"Artist"}};
}

// One track, keyed by a YouTube Music entry whose id YouTube Music no longer plays
core::State dead_entry_state() {
    core::State state;
    std::string key = ytm_key(DeadId);
    state.sources[key] = {.title = "Song", .artists = {"Artist"}, .track = key};
    state.tracks[key] = {.sources = {key}};
    state.playlists["ytm:list"] = {.sources = {key}};
    return state;
}

std::string resolved_text(const core::State& state, const std::string& track) {
    std::optional<core::ResolvedYtmId> resolved = core::resolved_ytm_id(state, state.tracks.at(track));
    return resolved ? std::format("{} from {}{}", resolved->ytm_id, resolved->source, resolved->matched ? ", matched" : "") : std::string("none");
}

void check_resolved(const core::State& state, std::string_view expected, std::source_location where = std::source_location::current()) {
    std::string actual = resolved_text(state, ytm_key(DeadId));
    check(actual == expected, std::format("resolved: {}, expected {}", actual, expected), where);
}

void check_resolution() {
    std::string key = ytm_key(DeadId);
    core::State state = dead_entry_state();
    // Not yet known to be unavailable: the playlist's choice
    check_resolved(state, std::format("{} from {}", DeadId, key));
    CHECK(!core::needs_match(state, state.tracks.at(key)));

    state.unavailable_ytm_ids.insert(std::string(DeadId));
    check_resolved(state, "none");
    CHECK(core::needs_match(state, state.tracks.at(key)));
    CHECK(core::is_matched_entry(state, key));
    CHECK(core::tracks_to_download(state).empty());

    state.sources.at(key).match = match_to(LiveId, 1.0);
    check_resolved(state, std::format("{} from {}, matched", LiveId, key));
    CHECK(!core::needs_match(state, state.tracks.at(key)));
    CHECK(!core::needs_review(state, state.tracks.at(key)));
    CHECK(core::tracks_to_download(state).size() == 1);

    // A doubtful match waits for review, as a Spotify track's does
    state.sources.at(key).match = match_to(LiveId, 0.75);
    CHECK(core::needs_review(state, state.tracks.at(key)));
    CHECK(core::tracks_to_download(state).empty());

    // A match to another unavailable id is matched again
    state.sources.at(key).match = match_to("otherDead01", 1.0);
    state.unavailable_ytm_ids.insert("otherDead01");
    check_resolved(state, "none");
}

void check_spotify_first() {
    // A track with a Spotify source is matched by it: its facts tell more
    core::State state = dead_entry_state();
    std::string key = ytm_key(DeadId);
    state.sources["spotify:abc"] = {.title = "Song", .artists = {"Artist"}, .album = "Album", .match = match_to(DeadId, 1.0), .track = key};
    state.tracks.at(key).sources.push_back("spotify:abc");
    state.unavailable_ytm_ids.insert(std::string(DeadId));
    const std::string* source = core::source_to_match(state, state.tracks.at(key));
    check(source != nullptr && *source == "spotify:abc", "the Spotify source is the one to match");
    check_resolved(state, "none");
}

void check_apply_unavailable() {
    core::State state = dead_entry_state();
    std::string key = ytm_key(DeadId);
    state.tracks.at(key).status = core::TrackStatus::Failed;
    state.tracks.at(key).failure = core::TrackFailure::Download;
    state.tracks.at(key).error = "earlier";
    // Another track waits for review with a match to the same id
    state.sources["spotify:other"] = {.match = match_to(DeadId, 0.75), .track = "spotify:other"};
    state.tracks["spotify:other"] = {.sources = {"spotify:other"}};

    CHECK(core::apply_unavailable(state, key, std::string(DeadId)));
    CHECK(state.unavailable_ytm_ids.contains(std::string(DeadId)));
    CHECK(state.tracks.at(key).status == core::TrackStatus::Pending);
    CHECK(state.tracks.at(key).failure == core::TrackFailure::None);
    CHECK(state.tracks.at(key).error.empty());
    CHECK(!state.sources.at("spotify:other").match.has_value());

    // The user's own choice fails as any download does
    core::State overridden = dead_entry_state();
    overridden.tracks.at(key).ytm_override = "chosenId001";
    CHECK(!core::apply_unavailable(overridden, key, "chosenId001"));
    CHECK(overridden.unavailable_ytm_ids.empty());
}

}

void check_unavailable() {
    check_errors();
    check_resolution();
    check_spotify_first();
    check_apply_unavailable();
}

}
