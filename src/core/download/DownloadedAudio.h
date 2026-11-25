#pragma once

#include <filesystem>
#include <optional>
#include <string>

namespace westie::core {

// What yt-dlp's info JSON knows of the song
struct VideoInfo {
    std::string album{};
    std::optional<int> year{};
};

struct DownloadedAudio {
    std::filesystem::path audio{};
    // Not every video has one
    std::optional<std::filesystem::path> thumbnail{};
    VideoInfo info{};
};

// Tagged in the work folder, waiting to move into the library
struct TaggedAudio {
    std::filesystem::path audio{};
    std::string stem{};
    VideoInfo info{};
};

}
