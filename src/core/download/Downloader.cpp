#include "core/download/Downloader.h"

#include <algorithm>
#include <array>
#include <cstddef>
#include <format>
#include <optional>
#include <ranges>
#include <string_view>
#include <utility>
#include <vector>

#include <glaze/glaze.hpp>

#include "core/download/DownloadFailed.h"
#include "core/download/DownloadProgress.h"
#include "core/json/JsonOptions.h"
#include "core/json/Utf8Path.h"
#include "core/tools/ToolPackage.h"

namespace westie::core {

namespace {

// yt-dlp runs ffmpeg to extract the audio, and deno to solve YouTube's challenges
constexpr std::array AlsoUses = {ToolPackage::Ffmpeg, ToolPackage::Deno};
constexpr std::string_view ErrorPrefix = "ERROR:";
constexpr std::size_t LastLines = 2;

struct InfoJson {
    std::optional<std::string> album{};
    std::optional<int> release_year{};
};

std::string_view extension_of(AudioFormat format) {
    switch (format) {
    case AudioFormat::Opus: return "opus";
    case AudioFormat::Flac: return "flac";
    }
    return "opus";
}

std::vector<std::string_view> lines_of(std::string_view text) {
    std::vector<std::string_view> lines;
    for (auto part : std::views::split(text, '\n')) {
        std::string_view line(part.begin(), part.end());
        if (line.ends_with('\r')) {
            line.remove_suffix(1);
        }
        if (!line.empty()) {
            lines.push_back(line);
        }
    }
    return lines;
}

// yt-dlp's errors, or what it printed last when it named none
std::string error_text(std::string_view errors, int exit_code) {
    std::vector<std::string_view> lines = lines_of(errors);
    std::vector<std::string_view> chosen;
    for (std::string_view line : lines) {
        if (line.starts_with(ErrorPrefix)) {
            chosen.push_back(line);
        }
    }
    if (chosen.empty()) {
        chosen.assign(lines.end() - static_cast<std::ptrdiff_t>(std::min(LastLines, lines.size())), lines.end());
    }
    if (chosen.empty()) {
        return std::format("yt-dlp exited with code {}", exit_code);
    }
    std::string text;
    for (std::string_view line : chosen) {
        if (!text.empty()) {
            text += '\n';
        }
        text += line;
    }
    return text;
}

}

}

template<>
struct glz::meta<westie::core::InfoJson> {
    using T = westie::core::InfoJson;
    constexpr static auto value = glz::object("album", &T::album, "release_year", &T::release_year);
};

namespace westie::core {

Downloader::Downloader(const YtDlp& yt_dlp, const ToolFolder& tools, const platform::FileOps& files) : yt_dlp_(yt_dlp), tools_(tools), files_(files) {}

DownloadedAudio Downloader::download(
        const std::string& ytm_id, AudioFormat format, const std::filesystem::path& work, const LogSubject& subject,
        const std::function<void(int percent)>& on_percent, const std::stop_token& stop
) const {
    std::string_view extension = extension_of(format);
    std::vector<std::string> arguments = {
        "-f",
        "bestaudio[acodec=opus]/bestaudio",
        "-x",
        "--audio-format",
        std::string(extension),
        "--write-info-json",
        "--write-thumbnail",
        "--convert-thumbnails",
        "jpg",
        "--ffmpeg-location",
        to_utf8(tools_.path()),
        "--js-runtimes",
        std::format("deno:{}", to_utf8(tools_.program(ToolPackage::Deno))),
        "--newline",
        "--progress-template",
        "download:[progress] %(progress._percent_str)s",
        "--no-playlist",
        "-o",
        to_utf8(work / L"%(id)s.%(ext)s"),
        std::format("https://music.youtube.com/watch?v={}", ytm_id),
    };

    std::optional<int> shown;
    platform::ProcessResult result = yt_dlp_.run(
            std::move(arguments), AlsoUses, subject,
            [&](std::string_view line) {
                // A line comes with every update; the UI needs only whole percents
                if (std::optional<int> percent = download_percent(line); percent && percent != shown) {
                    shown = percent;
                    on_percent(*percent);
                }
            },
            stop
    );
    if (result.exit_code != 0) {
        throw DownloadFailed(error_text(result.errors, result.exit_code));
    }

    DownloadedAudio downloaded{.audio = work / path_from_utf8(std::format("{}.{}", ytm_id, extension))};
    if (!files_.is_file(downloaded.audio)) {
        throw DownloadFailed(std::format("yt-dlp wrote no {} file", extension));
    }
    if (std::filesystem::path thumbnail = work / path_from_utf8(ytm_id + ".jpg"); files_.is_file(thumbnail)) {
        downloaded.thumbnail = thumbnail;
    }
    downloaded.info = read_info(work / path_from_utf8(ytm_id + ".info.json"));
    return downloaded;
}

// An info JSON that's missing or doesn't read only leaves the album and year out
VideoInfo Downloader::read_info(const std::filesystem::path& file) const {
    std::optional<std::string> contents = files_.read_file(file);
    InfoJson json;
    if (!contents || glz::read<JsonReadOptions>(json, *contents)) {
        return {};
    }
    return {.album = std::move(json.album).value_or(""), .year = json.release_year};
}

}
