#include <format>
#include <optional>
#include <set>
#include <source_location>
#include <string>
#include <string_view>
#include <vector>

#include "Checks.h"
#include "UnitChecks.h"
#include "core/download/DownloadProgress.h"
#include "core/library/LibraryFileName.h"
#include "core/library/WindowsNames.h"

namespace westie::tests {

namespace {

std::string_view problem_text(core::NameProblem problem) {
    switch (problem) {
    case core::NameProblem::None:                return "none";
    case core::NameProblem::EndsInSpaceOrPeriod: return "ends";
    case core::NameProblem::ReservedName:        return "reserved";
    }
    return "?";
}

void check_problem(std::string_view name, core::NameProblem expected, std::source_location where = std::source_location::current()) {
    core::NameProblem actual = core::windows_name_problem(name);
    check(actual == expected, std::format("\"{}\": {}, expected {}", name, problem_text(actual), problem_text(expected)), where);
}

void check_windows_names() {
    check_problem("Music", core::NameProblem::None);
    check_problem("Music ", core::NameProblem::EndsInSpaceOrPeriod);
    check_problem("Music.", core::NameProblem::EndsInSpaceOrPeriod);
    check_problem("CON", core::NameProblem::ReservedName);
    check_problem("con.txt", core::NameProblem::ReservedName);
    check_problem("Nul .opus", core::NameProblem::ReservedName);
    check_problem("COM1", core::NameProblem::ReservedName);
    check_problem("lpt9.flac", core::NameProblem::ReservedName);
    check_problem("COM0", core::NameProblem::None);
    check_problem("COM10", core::NameProblem::None);
    check_problem("Console", core::NameProblem::None);
    check_problem("AUX - Song", core::NameProblem::None);
}

void check_stem(
        std::vector<std::string> artists, std::string title, std::string_view expected, std::source_location where = std::source_location::current()
) {
    std::string actual = core::library_file_stem({.title = std::move(title), .artists = std::move(artists)}, "abcDEF12345");
    check(actual == expected, std::format("\"{}\", expected \"{}\"", actual, expected), where);
}

void check_stems() {
    check_stem({"Daniel Caesar", "H.E.R."}, "Best Part", "Daniel Caesar, H.E.R. - Best Part");
    check_stem({"AC/DC"}, "What? <Live>: \"Yes\"|*", "AC_DC - What_ _Live__ _Yes___");
    check_stem({"Tab\tBand"}, "Song...", "Tab_Band - Song");
    check_stem({}, "Only Title", "Only Title");
    check_stem({"Nobody"}, "", "abcDEF12345");
    check_stem({}, "CON", "CON_");
    check_stem({}, " . ", "abcDEF12345");
    std::string long_title(250, 'x');
    check_stem({}, long_title, std::string(200, 'x'));
    // 199 ASCII letters and then a two-byte letter fit; a cut never splits one
    std::string accents = std::string(199, 'a') + "\xC3\xA9\xC3\xA9";
    check_stem({}, accents, std::string(199, 'a') + "\xC3\xA9");
}

void check_free_names() {
    std::set<std::string> taken = {core::file_name_key("Artist - Song.opus"), core::file_name_key("ARTIST - SONG (2).OPUS")};
    std::string free = core::free_file_name("Artist - Song", "opus", taken);
    check(free == "Artist - Song (3).opus", std::format("free name: {}", free));
    free = core::free_file_name("Artist - Song", "flac", taken);
    check(free == "Artist - Song.flac", std::format("another extension: {}", free));
    free = core::free_file_name("Artist - Song", "opus", {}, 4);
    check(free == "Artist - Song (4).opus", std::format("from a number: {}", free));
}

void check_percent(std::string_view line, std::optional<int> expected, std::source_location where = std::source_location::current()) {
    std::optional<int> actual = core::download_percent(line);
    auto text = [](std::optional<int> percent) { return percent ? std::format("{}", *percent) : std::string("none"); };
    check(actual == expected, std::format("\"{}\": {}, expected {}", line, text(actual), text(expected)), where);
}

void check_progress() {
    check_percent("[progress]  45.3%", 45);
    check_percent("[progress]   0.0%", 0);
    check_percent("[progress] 100.0%", 100);
    check_percent("[progress]  99.9%", 99);
    check_percent("[progress] \x1b[0;94m 62.5%\x1b[0m", 62);
    check_percent("[progress]   N/A%", std::nullopt);
    check_percent("[progress] Unknown", std::nullopt);
    // What the template printed before: yt-dlp takes "download:" as the kind of progress, not as text
    check_percent("download: 45.3%", std::nullopt);
    check_percent("[download]  45.3% of 3.42MiB", std::nullopt);
    check_percent("[download] Destination: x.webm", std::nullopt);
    check_percent("45.3%", std::nullopt);
}

}

void check_library() {
    check_windows_names();
    check_stems();
    check_free_names();
    check_progress();
}

}
