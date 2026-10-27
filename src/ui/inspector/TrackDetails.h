#pragma once

#include <chrono>
#include <optional>
#include <string>
#include <vector>

#include "core/sources/SourceType.h"
#include "core/state/State.h"
#include "ui/tracks/TrackRows.h"

namespace westie::ui {

struct TrackSource {
    core::SourceType type = core::SourceType::YouTubeMusic;
    std::wstring url{};
};

struct TrackPlaylist {
    std::string key{};
    std::wstring name{};
};

enum class VideoChooser { Playlist, Matcher };

struct TrackVideo {
    std::wstring id{};
    std::wstring url{};
    VideoChooser chosen_by = VideoChooser::Playlist;
};

struct TrackMatch {
    std::wstring video_id{};
    std::wstring title{};
    std::wstring artists{};
    std::chrono::milliseconds duration{};
    // The Spotify source's, which the video's is held against
    std::chrono::milliseconds source_duration{};
    double confidence = 0.0;
};

struct TrackDetails {
    TrackRow row{};
    // The first source of each kind, in the order of AllSourceTypes
    std::vector<TrackSource> sources{};
    // In the left panel's order
    std::vector<TrackPlaylist> playlists{};
    std::optional<TrackVideo> video{};
    // The first Spotify source's
    std::optional<TrackMatch> match{};
    core::TrackFailure failure = core::TrackFailure::None;
    // Its last lines
    std::wstring error{};
};

std::optional<TrackDetails> track_details(const core::State& state, const TrackRow& row);

}
