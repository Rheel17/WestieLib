#include <format>
#include <optional>
#include <source_location>
#include <string>
#include <utility>

#include "Checks.h"
#include "UnitChecks.h"
#include "core/state/State.h"
#include "ui/main/MatchChoices.h"

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

core::Track failed(core::TrackFailure failure) {
    return {.status = core::TrackStatus::Failed, .failure = failure};
}

// What take gives until nothing waits, joined with " / "
std::string taken(ui::MatchChoices& choices, const core::State& state) {
    std::string text;
    while (std::optional<std::string> track = choices.take(state)) {
        text += std::format("{}{}", text.empty() ? "" : " / ", *track);
    }
    return text;
}

void check_taken(ui::MatchChoices& choices, const core::State& state, const std::string& expected, std::source_location where = std::source_location::current()) {
    std::string actual = taken(choices, state);
    check(actual == expected, std::format("taken: \"{}\", expected \"{}\"", actual, expected), where);
}

}

// Choose a match opens by itself for a track that needs review or has no match, once per update, in the order the
// update matched them
void check_match_choices() {
    core::State state;
    add(state, "spotify:review", {}, matched(0.75));
    add(state, "spotify:accepted", {}, matched(1.0));
    add(state, "spotify:confirmed", {}, matched(0.75, true));
    add(state, "spotify:noMatch", failed(core::TrackFailure::NoMatch));
    add(state, "spotify:search", failed(core::TrackFailure::Search));
    add(state, "spotify:download", failed(core::TrackFailure::Download), matched(0.75));
    add(state, "ytm:listed00001", {});

    CHECK(ui::needs_choice(state, "spotify:review") && ui::needs_choice(state, "spotify:noMatch"));
    CHECK(!ui::needs_choice(state, "spotify:accepted") && !ui::needs_choice(state, "spotify:confirmed"));
    // A failed search is tried again on the next update; a failed download has its match; a playlist's id isn't matched
    CHECK(!ui::needs_choice(state, "spotify:search") && !ui::needs_choice(state, "spotify:download") && !ui::needs_choice(state, "ytm:listed00001"));
    CHECK(!ui::needs_choice(state, "spotify:gone"));

    ui::MatchChoices choices;
    for (std::string track : {"spotify:noMatch", "spotify:accepted", "spotify:review", "spotify:search", "spotify:noMatch"}) {
        choices.add(state, track);
    }
    CHECK(choices.any_waiting(state));
    check_taken(choices, state, "spotify:noMatch / spotify:review");
    CHECK(!choices.any_waiting(state));

    // Once per update
    choices.add(state, "spotify:review");
    check_taken(choices, state, "");
    choices.start_update();
    choices.add(state, "spotify:review");
    choices.add(state, "spotify:noMatch");
    // Confirmed while it waited: left out
    state.sources.at("spotify:review").match->confirmed = true;
    CHECK(choices.any_waiting(state));
    check_taken(choices, state, "spotify:noMatch");

    choices.start_update();
    choices.add(state, "spotify:noMatch");
    choices.clear();
    CHECK(!choices.any_waiting(state));
    check_taken(choices, state, "");
}

}
