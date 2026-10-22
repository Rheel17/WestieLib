#pragma once

#include <string_view>

namespace westie::core {

std::string_view yt_dlp_error_message(std::string_view errors, std::string_view id);
bool says_playlist_unavailable(std::string_view message);

}
