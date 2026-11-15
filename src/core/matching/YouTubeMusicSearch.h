#pragma once

#include <chrono>
#include <optional>
#include <stop_token>
#include <string>
#include <string_view>
#include <vector>

#include "core/matching/SearchTimeout.h"
#include "platform/HttpClient.h"

namespace westie::core {

struct SongResult {
    std::string ytm_id{};
    std::string title{};
    std::vector<std::string> artists{};
    std::string album{};
    std::chrono::milliseconds duration{};
    std::optional<bool> explicit_content{};
};

class YouTubeMusicSearch {
public:
    YouTubeMusicSearch(platform::HttpClient& http, std::string api_url);

    std::vector<SongResult> songs(std::string_view query, const std::stop_token& stop);

private:
    platform::HttpClient& http_;
    std::string api_url_;
    SearchTimeout timeout_;
};

}
