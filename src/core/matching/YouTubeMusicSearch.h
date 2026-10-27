#pragma once

#include <chrono>
#include <stop_token>
#include <string>
#include <string_view>
#include <vector>

#include "platform/HttpClient.h"

namespace westie::core {

struct SongResult {
    std::string video_id{};
    std::string title{};
    std::vector<std::string> artists{};
    std::string album{};
    std::chrono::milliseconds duration{};
};

std::vector<SongResult> search_youtube_music_songs(platform::HttpClient& http, std::string_view api_url, std::string_view query, const std::stop_token& stop);

}
