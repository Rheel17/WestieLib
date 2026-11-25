#pragma once

#include <chrono>
#include <filesystem>
#include <optional>
#include <string>
#include <vector>

#include "core/sources/SourceType.h"
#include "core/state/MatchDoubt.h"
#include "core/state/State.h"
#include "platform/TextNormalizer.h"
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

enum class AudioChooser { Playlist, Matcher, Override };

struct TrackAudio {
    std::wstring id{};
    std::wstring url{};
    AudioChooser chosen_by = AudioChooser::Playlist;
};

struct TrackMatch {
    std::wstring ytm_id{};
    std::wstring title{};
    std::wstring artists{};
    std::chrono::milliseconds duration{};
    // The Spotify source's, which the result's is held against
    std::chrono::milliseconds source_duration{};
    double confidence = 0.0;
    std::vector<core::MatchDoubt> doubts{};
    std::optional<bool> explicit_content{};
    // The version words only one of the two titles has, as it writes them
    std::wstring source_words{};
    std::wstring result_words{};
};

struct TrackDetails {
    TrackRow row{};
    // The first source of each kind, in the order of AllSourceTypes
    std::vector<TrackSource> sources{};
    // In the left panel's order
    std::vector<TrackPlaylist> playlists{};
    // How the entry got it
    std::optional<TrackAudio> audio{};
    // The entry's own match, or for a YouTube Music entry its track's first Spotify source's
    std::optional<TrackMatch> match{};
    core::TrackFailure failure = core::TrackFailure::None;
    // Its last lines
    std::wstring error{};
    std::optional<int> year{};
    // In the library folder, empty without a file
    std::filesystem::path file{};
};

std::optional<TrackDetails> track_details(
        const core::State& state, const platform::TextNormalizer& normalizer, const TrackRow& row, const std::filesystem::path& library_folder
);

}
