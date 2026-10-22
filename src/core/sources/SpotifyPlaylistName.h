#pragma once

#include <optional>
#include <stop_token>
#include <string>
#include <string_view>

#include "platform/HttpClient.h"

namespace westie::core {

std::optional<std::string> spotify_playlist_name(platform::HttpClient& http, std::string_view spotify_url, std::string_view id, const std::stop_token& stop);

}
