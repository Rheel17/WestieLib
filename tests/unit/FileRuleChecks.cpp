#include <format>
#include <map>
#include <ranges>
#include <string>
#include <string_view>
#include <utility>

#include "Checks.h"
#include "UnitChecks.h"
#include "core/library/ApplyRemovals.h"
#include "core/library/FileChanges.h"
#include "core/state/MergeTracks.h"
#include "core/state/PruneTracks.h"
#include "core/state/State.h"

namespace westie::tests {

namespace {

constexpr std::string_view SharedId = "sharedId001";
constexpr std::string_view OtherId = "otherId0001";

void add(core::State& state, const std::string& key, core::Track track, core::Source source = {}) {
    source.track = key;
    track.sources.insert(track.sources.begin(), key);
    state.sources[key] = std::move(source);
    state.tracks[key] = std::move(track);
}

core::Source song(std::string title, std::string ytm_id = std::string(SharedId)) {
    return {.title = std::move(title), .artists = {"Band"}, .match = core::Match{.ytm_id = std::move(ytm_id), .confidence = 1.0}};
}

core::Track downloaded(std::string file, std::string_view ytm_id = SharedId) {
    return {.ytm_id = std::string(ytm_id), .status = core::TrackStatus::Downloaded, .file = std::move(file)};
}

void list_all(core::State& state) {
    for (const std::string& key : state.tracks | std::views::keys) {
        state.playlists["ytm:list"].sources.push_back(key);
    }
}

const std::string YtmKey = "ytm:" + std::string(SharedId);

void check_adopting() {
    core::State state;
    add(state, YtmKey, {}, {.title = "Song", .artists = {"Band"}});
    add(state, "spotify:song", downloaded("Band - Song.opus"), song("Song"));
    list_all(state);
    core::State before = state;
    core::merge_tracks(state, YtmKey, "spotify:song");
    const core::Track& survivor = state.tracks.at(YtmKey);
    bool adopted = survivor.file == "Band - Song.opus" && survivor.status == core::TrackStatus::Downloaded && survivor.ytm_id == SharedId;
    check(adopted, std::format("a survivor without a file takes the other's: \"{}\"", survivor.file.string()));
    core::FileChanges changes = core::file_changes(before, state);
    check(changes.dropped.empty(), "an adopted file isn't deleted");
    bool retagged = changes.retagged.size() == 1 && changes.retagged.front().track == YtmKey && !changes.retagged.front().stem;
    check(retagged, "an adopted file gets its new sources in the tags, under the same name");
}

void check_both_files() {
    core::State state;
    add(state, YtmKey, downloaded("Band - Song.opus"), {.title = "Song", .artists = {"Band"}});
    add(state, "spotify:song", downloaded("Band - Song (2).opus"), song("Song"));
    list_all(state);
    core::State before = state;
    core::merge_tracks(state, YtmKey, "spotify:song");
    check(state.tracks.at(YtmKey).file == "Band - Song.opus", "with two files, the survivor keeps its own");
    core::FileChanges changes = core::file_changes(before, state);
    bool dropped = changes.dropped.size() == 1 && changes.dropped.front().file == "Band - Song (2).opus" && changes.dropped.front().track == "spotify:song";
    check(dropped, "the merged track's file is deleted");
}

void check_file_of_the_shared_id() {
    core::State state;
    add(state, YtmKey, {.ytm_id = std::string(OtherId), .status = core::TrackStatus::Pending, .file = "Old.opus"}, {.title = "Song", .artists = {"Band"}});
    add(state, "spotify:song", downloaded("Band - Song.opus"), song("Song"));
    list_all(state);
    core::merge_tracks(state, YtmKey, "spotify:song");
    const core::Track& survivor = state.tracks.at(YtmKey);
    bool taken = survivor.file == "Band - Song.opus" && survivor.status == core::TrackStatus::Downloaded;
    check(taken, "a survivor whose file has another id takes the one made from the shared id");
}

void check_returning() {
    core::State state;
    add(state, "spotify:back", {.ytm_id = std::string(SharedId), .status = core::TrackStatus::Orphaned, .file = "Back.opus"}, song("Back"));
    add(state, "spotify:replaced", {.ytm_id = std::string(OtherId), .status = core::TrackStatus::Kept, .file = "Replaced.opus"}, song("Replaced"));
    add(state, "spotify:left", {.ytm_id = std::string(OtherId), .status = core::TrackStatus::Pending, .file = "Left.opus"}, song("Left"));
    add(state, "spotify:kept", {.ytm_id = std::string(SharedId), .status = core::TrackStatus::Kept, .file = "Kept.opus"}, song("Kept"));
    add(state, "spotify:fileless", {}, song("Fileless"));
    state.playlists["spotify:list"].sources = {"spotify:back", "spotify:replaced"};
    core::prune_tracks(state);
    check(state.tracks.at("spotify:back").status == core::TrackStatus::Downloaded, "a Removed track back in a playlist is Downloaded");
    check(state.tracks.at("spotify:replaced").status == core::TrackStatus::Pending, "one whose file has another id than it has now downloads again");
    check(state.tracks.at("spotify:left").status == core::TrackStatus::Orphaned, "a track waiting to download again that leaves is Removed, file and all");
    check(state.tracks.at("spotify:kept").status == core::TrackStatus::Kept, "a Kept track out of the playlists stays Kept");
    check(!state.tracks.contains("spotify:fileless") && !state.sources.contains("spotify:fileless"), "a track without a file goes with its sources");
}

void check_removals() {
    core::State state;
    add(state, "spotify:orphan", {.status = core::TrackStatus::Orphaned, .file = "Orphan.opus"}, song("Orphan"));
    add(state, "spotify:kept", {.status = core::TrackStatus::Kept, .file = "Kept.opus"}, song("Kept"));
    add(state, "spotify:listed", downloaded("Listed.opus"), song("Listed"));
    core::State before = state;
    std::map<std::string, core::RemovalDecision> applied = core::apply_removals(
            state,
            {
                {"spotify:orphan", core::RemovalDecision::Keep},
                {"spotify:kept", core::RemovalDecision::Remove},
                {"spotify:listed", core::RemovalDecision::Remove},
                {"spotify:gone", core::RemovalDecision::Remove},
            }
    );
    check(applied.size() == 2, std::format("decisions applied: {}, expected 2", applied.size()));
    check(state.tracks.at("spotify:orphan").status == core::TrackStatus::Kept, "Keep makes a Removed track Kept");
    check(!state.tracks.contains("spotify:kept") && !state.sources.contains("spotify:kept"), "Remove deletes a Kept track with its sources");
    check(state.tracks.at("spotify:listed").status == core::TrackStatus::Downloaded, "a track in a playlist isn't removed");
    core::FileChanges changes = core::file_changes(before, state);
    check(changes.dropped.size() == 1 && changes.dropped.front().file == "Kept.opus", "the removed track's file is deleted");
}

void check_renames() {
    core::State state;
    add(state, "spotify:song", downloaded("Band - Old.opus"), song("Old"));
    list_all(state);
    core::State before = state;
    state.sources.at("spotify:song").title = "New";
    core::FileChanges changes = core::file_changes(before, state);
    bool renamed = changes.retagged.size() == 1 && changes.retagged.front().stem == "Band - New" && changes.retagged.front().tags.title == "New";
    check(renamed, "a new title re-tags and renames the file");

    core::State replaced = before;
    replaced.tracks.at("spotify:song").ytm_id = std::string(OtherId);
    replaced.sources.at("spotify:song").title = "New";
    check(core::file_changes(before, replaced).retagged.empty(), "a file a download just replaced isn't re-tagged");
    core::FileChanges none = core::file_changes(before, before);
    check(none.retagged.empty() && none.dropped.empty(), "no change, no file work");
}

}

void check_file_rules() {
    check_adopting();
    check_both_files();
    check_file_of_the_shared_id();
    check_returning();
    check_removals();
    check_renames();
}

}
