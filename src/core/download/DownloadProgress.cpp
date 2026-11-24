#include "core/download/DownloadProgress.h"

#include <algorithm>
#include <charconv>
#include <cmath>
#include <system_error>

namespace westie::core {

namespace {

// What --progress-template "download:%(progress._percent_str)s" puts before the percentage
constexpr std::string_view Prefix = "download:";
constexpr int FullPercent = 100;

}

std::optional<int> download_percent(std::string_view line) {
    if (!line.starts_with(Prefix)) {
        return std::nullopt;
    }
    line.remove_prefix(Prefix.size());
    // yt-dlp pads the percentage to a width
    while (line.starts_with(' ')) {
        line.remove_prefix(1);
    }
    double percent = 0;
    auto [end, error] = std::from_chars(line.data(), line.data() + line.size(), percent);
    if (error != std::errc() || std::string_view(end, line.data() + line.size()) != "%" || !std::isfinite(percent)) {
        return std::nullopt;
    }
    return std::clamp(static_cast<int>(std::floor(percent)), 0, FullPercent);
}

}
