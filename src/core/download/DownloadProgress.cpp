#include "core/download/DownloadProgress.h"

#include <algorithm>
#include <cctype>
#include <charconv>
#include <cmath>
#include <cstddef>
#include <string>
#include <system_error>

namespace westie::core {

namespace {

// What --progress-template "download:[progress] %(progress._percent_str)s" prints before the percentage; the
// "download:" only says which progress the template is for
constexpr std::string_view Prefix = "[progress] ";
constexpr char Escape = '\x1b';
constexpr int FullPercent = 100;

// yt-dlp colors the percentage when it thinks it writes to a terminal
std::string without_colors(std::string_view text) {
    std::string plain;
    for (std::size_t index = 0; index < text.size(); ++index) {
        if (text[index] != Escape) {
            plain += text[index];
            continue;
        }
        // A sequence ends with its first letter, as in ESC [ 0 ; 3 4 m
        while (index + 1 < text.size() && !std::isalpha(static_cast<unsigned char>(text[index + 1]))) {
            ++index;
        }
        ++index;
    }
    return plain;
}

}

std::optional<int> download_percent(std::string_view line) {
    if (!line.starts_with(Prefix)) {
        return std::nullopt;
    }
    std::string text = without_colors(line.substr(Prefix.size()));
    std::string_view percent_text(text);
    // yt-dlp pads the percentage to a width
    while (percent_text.starts_with(' ')) {
        percent_text.remove_prefix(1);
    }
    while (percent_text.ends_with(' ')) {
        percent_text.remove_suffix(1);
    }
    double percent = 0;
    auto [end, error] = std::from_chars(percent_text.data(), percent_text.data() + percent_text.size(), percent);
    if (error != std::errc() || std::string_view(end, percent_text.data() + percent_text.size()) != "%" || !std::isfinite(percent)) {
        return std::nullopt;
    }
    return std::clamp(static_cast<int>(std::floor(percent)), 0, FullPercent);
}

}
