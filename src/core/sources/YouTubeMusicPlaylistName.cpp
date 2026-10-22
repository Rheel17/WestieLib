#include "core/sources/YouTubeMusicPlaylistName.h"

#include <format>
#include <stdexcept>
#include <string_view>
#include <utility>

#include <glaze/glaze.hpp>

#include "core/json/JsonOptions.h"
#include "core/sources/YtDlpError.h"
#include "platform/ProcessRunner.h"

namespace westie::core {

namespace {

// The part of yt-dlp's playlist JSON the look-up uses
struct PlaylistJson {
    std::string title{};
};

}

std::optional<std::string> youtube_music_playlist_name(const YtDlp& yt_dlp, const PlaylistLink& link, const std::stop_token& stop) {
    // Without entries, yt-dlp reads only the playlist's first page
    platform::ProcessResult result = yt_dlp.run({"--flat-playlist", "--dump-single-json", "--playlist-items", "0", link.url()}, stop);
    if (result.exit_code != 0) {
        std::string_view error = yt_dlp_error_message(result.errors, link.id);
        if (says_playlist_unavailable(error)) {
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
