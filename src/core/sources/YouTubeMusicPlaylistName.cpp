#include "core/sources/YouTubeMusicPlaylistName.h"

#include <algorithm>
#include <array>
#include <format>
#include <ranges>
#include <stdexcept>
#include <string_view>
#include <utility>

#include <glaze/glaze.hpp>

#include "core/json/JsonOptions.h"
#include "platform/ProcessRunner.h"

namespace westie::core {

namespace {

// The part of yt-dlp's playlist JSON the look-up uses
struct PlaylistJson {
    std::string title{};
};

constexpr std::string_view ErrorPrefix = "ERROR: ";
constexpr std::string_view IdSeparator = ": ";
// What YouTube says about a playlist that doesn't exist, and about one that is private
constexpr std::array<std::string_view, 2> UnavailableMessages = {"does not exist", "private"};

// yt-dlp's last error, without the "ERROR: [youtube:tab] <id>: " in front of it
std::string_view error_message(std::string_view errors, std::string_view id) {
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

}

std::optional<std::string> youtube_music_playlist_name(const YtDlp& yt_dlp, const PlaylistLink& link, const std::stop_token& stop) {
    // Without entries, yt-dlp reads only the playlist's first page
    platform::ProcessResult result = yt_dlp.run({"--flat-playlist", "--dump-single-json", "--playlist-items", "0", link.url()}, stop);
    if (result.exit_code != 0) {
        std::string_view error = error_message(result.errors, link.id);
        if (std::ranges::any_of(UnavailableMessages, [error](std::string_view unavailable) { return error.contains(unavailable); })) {
            return std::nullopt;
        }
        if (error.empty()) {
            throw std::runtime_error(std::format("yt-dlp exited with code {}", result.exit_code));
        }
        throw std::runtime_error(std::string(error));
    }

    PlaylistJson json;
    if (glz::error_ctx error = glz::read<JsonReadOptions>(json, result.output)) {
        throw std::runtime_error(std::format("yt-dlp's answer doesn't read: {}", glz::format_error(error, result.output)));
    }
    return std::move(json.title);
}

}
