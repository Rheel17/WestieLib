#pragma once

#include <optional>
#include <stop_token>
#include <string>
#include <string_view>

#include "platform/HttpClient.h"

namespace westie::core {

struct SpotifyEmbedPage {
    std::string name{};
    std::string access_token{};
};

std::optional<SpotifyEmbedPage> read_spotify_embed_page(platform::HttpClient& http, std::string_view spotify_url, std::string_view id, const std::stop_token& stop);

}
