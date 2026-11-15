#pragma once

#include <chrono>
#include <optional>
#include <string>

#include "core/state/State.h"
#include "ui/dialogs/LinkLine.h"

namespace westie::ui {

// What the Choose a match dialog opens with, for one Spotify entry
struct MatchChoice {
    std::string source{};
    // The logo and the title, then the artists, album and length in gray
    LinkLine heading{};
    std::wstring current{};
    std::wstring query{};
    std::chrono::milliseconds duration{};
};

std::optional<MatchChoice> match_choice(const core::State& state, const std::string& source_key);
// The title of the YouTube Music source with that id
std::optional<std::string> library_result_title(const core::State& state, const std::string& ytm_id);

}
