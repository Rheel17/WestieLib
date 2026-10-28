#pragma once

#include <chrono>
#include <optional>
#include <string>

#include "core/state/State.h"
#include "ui/dialogs/LinkLine.h"

namespace westie::ui {

// What the Choose a video dialog opens with, for one Spotify entry
struct VideoChoice {
    std::string source{};
    // The logo and the title, then the artists, album and length in gray
    LinkLine heading{};
    std::wstring current{};
    std::wstring query{};
    std::chrono::milliseconds duration{};
};

std::optional<VideoChoice> video_choice(const core::State& state, const std::string& source_key);
// The title of a YouTube Music source with the video
std::optional<std::string> library_video_title(const core::State& state, const std::string& video_id);

}
