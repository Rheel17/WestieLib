#pragma once

#include <chrono>
#include <filesystem>

namespace westie::core {

enum class AudioFormat { Opus, Flac };

constexpr int MinConcurrency = 1;
constexpr int MaxConcurrency = 4;
// What the Settings dialog offers for the pause between tracks
constexpr int MaxDelaySeconds = 60;

struct Settings {
    std::filesystem::path library_path{};
    AudioFormat format = AudioFormat::Opus;
    bool embed_cover = true;
    int concurrency = 2;
    std::chrono::seconds delay_between_tracks{1};
    bool auto_update_tools = true;
    // Choose a match opens by itself for a track an update leaves to review or unmatched
    bool open_choose_match = true;

    bool operator==(const Settings&) const = default;
};

}
