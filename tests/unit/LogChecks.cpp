#include <chrono>
#include <format>
#include <source_location>
#include <string>
#include <string_view>
#include <vector>

#include "Checks.h"
#include "UnitChecks.h"
#include "core/log/LogText.h"
#include "core/matching/MatchText.h"

namespace westie::tests {

namespace {

using LocalTime = std::chrono::local_time<std::chrono::milliseconds>;

LocalTime local_time(int year, unsigned month, unsigned day, int hours, int minutes, int seconds, int milliseconds) {
    return std::chrono::local_days(std::chrono::year(year) / std::chrono::month(month) / std::chrono::day(day)) + std::chrono::hours(hours) +
           std::chrono::minutes(minutes) + std::chrono::seconds(seconds) + std::chrono::milliseconds(milliseconds);
}

void check_text(std::string_view actual, std::string_view expected, std::source_location where = std::source_location::current()) {
    check(actual == expected, std::format("\"{}\", expected \"{}\"", actual, expected), where);
}

void check_times() {
    check_text(core::format_log_time(local_time(2026, 11, 12, 22, 34, 12, 87)), "2026-11-12 22:34:12.087");
    check_text(core::format_log_time(local_time(2026, 1, 2, 3, 4, 5, 6)), "2026-01-02 03:04:05.006");
    check_text(core::format_log_time(local_time(2026, 11, 12, 0, 0, 0, 0)), "2026-11-12 00:00:00.000");
    check_text(core::format_log_time(local_time(2025, 12, 31, 23, 59, 59, 999)), "2025-12-31 23:59:59.999");
    check(core::log_file_name(local_time(2026, 11, 12, 22, 34, 12, 871)) == L"2026-11-12_22-34-12.log", "log file name");
}

void check_file_lines() {
    LocalTime time = local_time(2026, 11, 12, 15, 12, 3, 871);
    check_text(core::log_file_line({.time = time, .level = core::LogLevel::Info, .text = "Update started: 4 playlists"}), "2026-11-12 15:12:03.871 INFO    Update started: 4 playlists\n");
    check_text(
            core::log_file_line({
                .time = time,
                .level = core::LogLevel::Error,
                .text = "Couldn't fetch the playlist: HTTP 404",
                .subject = {.playlist = "spotify:5aX", .name = "Slow & bluesy"},
            }),
            "2026-11-12 15:12:03.871 ERROR   Couldn't fetch the playlist: HTTP 404 [Slow & bluesy; playlist spotify:5aX]\n"
    );
    check_text(
            core::log_file_line({.time = time, .level = core::LogLevel::Warning, .text = "Needs review", .subject = {.track = "spotify:7yq"}}),
            "2026-11-12 15:12:03.871 WARNING Needs review [track spotify:7yq]\n"
    );
    check_text(
            core::log_file_line({.time = time, .level = core::LogLevel::Debug, .text = "First\r\nSecond\nThird"}),
            "2026-11-12 15:12:03.871 DEBUG   First\n    Second\n    Third\n"
    );
}

void check_tool_runs() {
    LocalTime time = local_time(2026, 11, 12, 15, 12, 3, 871);
    std::vector<std::string> arguments = {"--ignore-config", "--dump-single-json", "a b", ""};
    check_text(core::command_line("yt-dlp", arguments), "yt-dlp --ignore-config --dump-single-json \"a b\" \"\"");
    check_text(
            core::log_tool_run(time, "yt-dlp", arguments, {.exit_code = 0, .output = "{\"id\": 1}", .errors = "WARNING: slow\n"}),
            "2026-11-12 15:12:03.871 TOOL    yt-dlp --ignore-config --dump-single-json \"a b\" \"\"\n"
            "    exit code 0\n"
            "    stderr:\n"
            "        WARNING: slow\n"
            "    stdout: 9 bytes\n"
    );
    check_text(
            core::log_tool_run(time, "yt-dlp", {}, {.exit_code = 1, .output = "partial\nanswer", .errors = ""}),
            "2026-11-12 15:12:03.871 TOOL    yt-dlp\n"
            "    exit code 1\n"
            "    stdout:\n"
            "        partial\n"
            "        answer\n"
    );
}

void check_match_texts() {
    core::Match match{.confidence = 0.915, .title = "Redbone", .artists = {"Childish Gambino"}};
    check_text(core::match_text(match), "\"Redbone\" by Childish Gambino (92%)");
    match.artists = {"Daniel Caesar", "H.E.R."};
    check_text(core::match_text(match), "\"Redbone\" by Daniel Caesar, H.E.R. (92%)");
    match.artists.clear();
    check_text(core::match_text(match), "\"Redbone\" (92%)");
    check_text(core::doubts_text({}), "");
    check_text(core::doubts_text({core::MatchDoubt::Version, core::MatchDoubt::Length}), "doubts: version, length");
}

}

void check_log_texts() {
    check_times();
    check_file_lines();
    check_tool_runs();
    check_match_texts();
}

}
