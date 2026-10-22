#include "core/sources/YouTubeMusicPlaylist.h"

#include <algorithm>
#include <array>
#include <chrono>
#include <cmath>
#include <format>
#include <optional>
#include <stdexcept>
#include <string>
#include <string_view>
#include <utility>
#include <vector>

#include <glaze/glaze.hpp>

#include "core/json/JsonOptions.h"
#include "core/sources/SourceType.h"
#include "core/sources/YtDlpError.h"
#include "platform/ProcessRunner.h"

namespace westie::core {

namespace {

// The parts of yt-dlp's flat playlist JSON the fetch uses
struct EntryJson {
    std::string id{};
    std::string title{};
    std::optional<double> duration{};
    std::optional<std::string> channel{};
};

struct PlaylistJson {
    std::string title{};
    std::vector<EntryJson> entries{};
};

// Songs from an auto-generated channel carry it after the artist's name
constexpr std::string_view TopicSuffix = " - Topic";
// What the playlist lists in place of a video that can't be played anymore
constexpr std::array<std::string_view, 2> GoneTitles = {"[Private video]", "[Deleted video]"};
constexpr double MillisecondsPerSecond = 1000.0;

std::vector<std::string> artists_of(std::optional<std::string> channel) {
    if (!channel || channel->empty()) {
        return {};
    }
    if (channel->ends_with(TopicSuffix)) {
        channel->resize(channel->size() - TopicSuffix.size());
    }
    return {std::move(*channel)};
}

}

PlaylistContents fetch_youtube_music_playlist(const YtDlp& yt_dlp, const PlaylistLink& link, const std::stop_token& stop) {
    platform::ProcessResult result = yt_dlp.run({"--flat-playlist", "--dump-single-json", link.url()}, stop);
    if (result.exit_code != 0) {
        std::string_view error = yt_dlp_error_message(result.errors, link.id);
        if (says_playlist_unavailable(error)) {
            throw std::runtime_error("The playlist doesn't exist or is private.");
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

    PlaylistContents contents{.name = std::move(json.title)};
    for (EntryJson& entry : json.entries) {
        if (entry.id.empty() || std::ranges::contains(GoneTitles, entry.title)) {
            continue;
        }
        auto milliseconds = static_cast<std::chrono::milliseconds::rep>(std::lround(entry.duration.value_or(0.0) * MillisecondsPerSecond));
        contents.tracks.push_back({
            .key = std::format("{}:{}", source_prefix(SourceType::YouTubeMusic), entry.id),
            .title = std::move(entry.title),
            .artists = artists_of(std::move(entry.channel)),
            .duration = std::chrono::milliseconds(milliseconds),
        });
    }
    return contents;
}

}
