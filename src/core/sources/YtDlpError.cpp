#include "core/sources/YtDlpError.h"

#include <algorithm>
#include <array>
#include <ranges>

namespace westie::core {

namespace {

constexpr std::string_view ErrorPrefix = "ERROR: ";
constexpr std::string_view IdSeparator = ": ";
// What YouTube says about a playlist that doesn't exist, and about one that is private
constexpr std::array<std::string_view, 2> UnavailableMessages = {"does not exist", "private"};

}

// yt-dlp's last error, without the "ERROR: [youtube:tab] <id>: " in front of it
std::string_view yt_dlp_error_message(std::string_view errors, std::string_view id) {
    std::string_view message;
    for (auto line : std::views::split(errors, '\n')) {
        std::string_view text(line.begin(), line.end());
        if (text.starts_with(ErrorPrefix)) {
            message = text.substr(ErrorPrefix.size());
        }
    }
    if (message.ends_with('\r')) {
        message.remove_suffix(1);
    }
    if (std::size_t extractor_end = message.find("] "); message.starts_with('[') && extractor_end != std::string_view::npos) {
        message.remove_prefix(extractor_end + 2);
    }
    if (message.starts_with(id) && message.substr(id.size()).starts_with(IdSeparator)) {
        message.remove_prefix(id.size() + IdSeparator.size());
    }
    return message;
}

bool says_playlist_unavailable(std::string_view message) {
    return std::ranges::any_of(UnavailableMessages, [message](std::string_view unavailable) { return message.contains(unavailable); });
}

}
