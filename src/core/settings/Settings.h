#pragma once

#include <chrono>
#include <filesystem>

namespace westie::core {

enum class AudioFormat { Opus, Flac };

constexpr int MinConcurrency = 1;
constexpr int MaxConcurrency = 4;

struct Settings {
    std::filesystem::path library_path{};
    AudioFormat format = AudioFormat::Opus;
    bool embed_cover = true;
    int concurrency = 2;
    std::chrono::seconds delay_between_tracks{3};
    bool auto_update_tools = true;

    bool operator==(const Settings&) const = default;
};

}
