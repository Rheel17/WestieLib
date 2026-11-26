#include <chrono>
#include <format>
#include <optional>
#include <source_location>
#include <string>
#include <string_view>

#include "Checks.h"
#include "UnitChecks.h"
#include "platform/win32/Utf16.h"
#include "ui/playlists/LastSyncLine.h"
#include "ui/sync/SyncBoard.h"

namespace westie::tests {

namespace {

void check_text(std::wstring_view actual, std::wstring_view expected, std::source_location where = std::source_location::current()) {
    check(actual == expected, std::format("\"{}\", expected \"{}\"", platform::win32::to_utf8(actual), platform::win32::to_utf8(expected)), where);
}

std::string progress_text(const std::optional<ui::StageProgress>& progress) {
    return progress ? std::format("{} of {}", progress->ended, progress->queued) : "none";
}

void check_progress(const ui::SyncBoard& board, const std::string& playlist, std::string_view expected, std::source_location where = std::source_location::current()) {
    std::string actual = progress_text(board.match_progress(playlist));
    check(actual == expected, std::format("{}: {}, expected {}", playlist, actual, expected), where);
}

// A YouTube Music playlist and two Spotify ones; a track both Spotify playlists list counts under each
void check_board() {
    ui::SyncBoard board;
    board.apply(core::SyncQueued{.playlists = {"ytm:A", "spotify:B", "spotify:C"}});
    CHECK(board.running() && board.queued("spotify:B"));
    board.apply(core::PlaylistFetching{.playlist = "ytm:A"});
    CHECK(board.current() == "ytm:A" && !board.queued("ytm:A") && !board.matching());
    for (std::string playlist : {"ytm:A", "spotify:B", "spotify:C"}) {
        board.apply(core::PlaylistFetching{.playlist = playlist});
        board.finish_playlist(playlist);
    }
    CHECK(!board.matching());

    board.apply(
            core::TracksQueued{
                .tracks = {{.track = "t1", .playlists = {"spotify:B"}}, {.track = "t2", .playlists = {"spotify:B", "spotify:C"}}, {.track = "t3", .playlists = {"spotify:C"}}}
            }
    );
    CHECK(board.matching() && board.finished() == 0 && board.total() == 3);
    check_progress(board, "ytm:A", "none");
    check_progress(board, "spotify:B", "0 of 2");
    check_progress(board, "spotify:C", "0 of 2");
    CHECK(!board.matching_now("spotify:B"));

    board.apply(core::TrackMatching{.track = "t1"});
    CHECK(board.matching_now("spotify:B") && !board.matching_now("spotify:C"));
    board.finish_track("t1");
    check_progress(board, "spotify:B", "1 of 2");
    // Between its tracks it keeps spinning, until a track of another playlist starts
    CHECK(board.matching_now("spotify:B"));

    // Refused: it is matched again later, and nothing ends it in between
    board.apply(core::TrackMatching{.track = "t2"});
    board.apply(core::TrackMatching{.track = "t3"});
    CHECK(board.matching_now("spotify:C") && !board.matching_now("spotify:B"));
    board.finish_track("t3");
    board.apply(core::TrackMatching{.track = "t2"});
    CHECK(board.matching_now("spotify:B") && board.matching_now("spotify:C"));
    board.finish_track("t2");
    check_progress(board, "spotify:B", "none");
    check_progress(board, "spotify:C", "none");
    // A track the run didn't queue ends nothing
    board.finish_track("t9");
    CHECK(board.finished() == 3);

    // Downloads follow the matches: their own counts, the tracks downloading now counting too
    board.apply(core::DownloadsQueued{.tracks = {{.track = "t1", .playlists = {"spotify:B"}}, {.track = "t2", .playlists = {"spotify:B", "spotify:C"}}}});
    CHECK(board.downloading() && !board.matching() && board.finished() == 0 && board.under_way() == 0 && board.total() == 2);
    CHECK(board.download_progress("spotify:B") && board.download_progress("spotify:B")->queued == 2 && board.downloading_in("spotify:B") == 0);
    board.apply(core::TrackDownloading{.track = "t1"});
    board.apply(core::TrackDownloading{.track = "t2"});
    board.apply(core::TrackProgress{.track = "t2", .percent = 62});
    CHECK(board.under_way() == 2 && board.downloading_in("spotify:B") == 2 && board.downloading_in("spotify:C") == 1);
    ui::TrackActivities activities = board.activities();
    CHECK(activities.size() == 2 && activities["t1"].percent == 0 && activities["t2"].percent == 62);
    board.finish_track("t1");
    CHECK(board.finished() == 1 && board.under_way() == 1 && board.download_progress("spotify:B") && board.download_progress("spotify:B")->ended == 1);
    // An ended download keeps its ring until rows asked for after it ended show; one that ends meanwhile keeps it longer
    CHECK(board.activities()["t1"].percent == 0);
    board.rows_requested();
    board.finish_track("t2");
    CHECK(board.activities()["t1"].percent == 0 && board.activities()["t2"].percent == 62);
    board.rows_shown();
    CHECK(!board.activities().contains("t1") && board.activities()["t2"].percent == 62);
    board.rows_requested();
    board.rows_shown();
    CHECK(board.activities().empty());

    // Waiting for tools: the run, and the downloads that haven't started
    board.apply(core::DownloadsQueued{.tracks = {{.track = "t4", .playlists = {"spotify:C"}}}});
    board.apply(core::DownloadsWaiting{.waiting_for = {core::ToolPackage::Ffmpeg}});
    CHECK(board.waiting_for().size() == 1 && board.activities()["t4"].waiting_for.size() == 1 && board.activities()["t2"].waiting_for.empty());
    board.apply(core::DownloadsWaiting{});
    CHECK(board.waiting_for().empty() && !board.activities().contains("t4"));
    // A playlist queued meanwhile is synced first
    board.apply(core::SyncQueued{.playlists = {"ytm:D"}});
    board.apply(core::PlaylistFetching{.playlist = "ytm:D"});
    CHECK(!board.downloading() && board.current() == "ytm:D");
    board.finish_playlist("ytm:D");
    CHECK(board.downloading() && board.total() == 3);

    board.finish();
    CHECK(!board.running() && !board.matching() && !board.downloading() && board.total() == 0);
    check_progress(board, "spotify:C", "none");
}

void check_lines() {
    std::chrono::system_clock::time_point now = std::chrono::system_clock::now();
    ui::PlaylistRow row{.run = ui::PlaylistRun::Matching, .matched = 3, .to_match = 10, .matching_now = true};
    ui::LastSyncLine line = ui::last_sync_line(row, now);
    check_text(line.text, L"Matching 4 of 10");
    CHECK(line.control == ui::SyncControl::Spinning && line.mark == ui::SyncMark::None);
    row.matching_now = false;
    line = ui::last_sync_line(row, now);
    check_text(line.text, L"Matching 3 of 10");
    CHECK(line.control == ui::SyncControl::Queued);

    row = {.run = ui::PlaylistRun::Syncing};
    line = ui::last_sync_line(row, now);
    check_text(line.text, L"Syncing…");
    CHECK(line.control == ui::SyncControl::Spinning);
    row.waiting_for = {core::ToolPackage::YtDlp};
    line = ui::last_sync_line(row, now);
    check_text(line.text, L"Waiting for yt-dlp");
    CHECK(line.mark == ui::SyncMark::WaitingForTools && line.control == ui::SyncControl::Queued);
    check_text(ui::last_sync_line({.run = ui::PlaylistRun::Queued}, now).text, L"Queued");

    row = {.run = ui::PlaylistRun::Downloading, .downloaded = 5, .to_download = 22, .downloading_now = 2};
    line = ui::last_sync_line(row, now);
    check_text(line.text, L"Downloading 7 of 22");
    CHECK(line.control == ui::SyncControl::Spinning && line.mark == ui::SyncMark::None);
    row.waiting_for = {core::ToolPackage::Ffmpeg, core::ToolPackage::Deno};
    line = ui::last_sync_line(row, now);
    check_text(line.text, L"Waiting for ffmpeg and deno");
    CHECK(line.mark == ui::SyncMark::WaitingForTools && line.control == ui::SyncControl::Queued);

    line = ui::last_sync_line({}, now);
    check_text(line.text, L"Never updated");
    CHECK(line.mark == ui::SyncMark::Unsynced && line.control == ui::SyncControl::Idle);
    line = ui::last_sync_line({.last_sync = now}, now);
    CHECK(line.text.starts_with(L"Updated today ") && line.mark == ui::SyncMark::Synced);
    line = ui::last_sync_line({.last_sync = now, .fetch_failed_at = now}, now);
    check_text(line.warning, L"Couldn't sync");
    CHECK(line.text.starts_with(L", updated today ") && line.mark == ui::SyncMark::FetchFailed);
}

}

void check_run_phases() {
    check_board();
    check_lines();
}

}
