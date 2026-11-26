#include "core/download/UnavailableError.h"

#include <algorithm>
#include <array>
#include <ranges>

namespace westie::core {

namespace {

constexpr std::string_view ErrorPrefix = "ERROR:";
// What yt-dlp says when YouTube Music doesn't play an id here, whoever asks; an age or bot check isn't among them
constexpr std::array<std::string_view, 5> UnavailableWords = {
    "Video unavailable",
    "This video is not available",
    "not made this video available in your country",
    "This video has been removed",
    "only available to Music Premium members",
};

}

bool is_unavailable_error(std::string_view error) {
    for (auto part : std::views::split(error, '\n')) {
        std::string_view line(part.begin(), part.end());
        if (line.starts_with(ErrorPrefix) && std::ranges::any_of(UnavailableWords, [&](std::string_view words) { return line.contains(words); })) {
            return true;
        }
    }
    return false;
}

}
