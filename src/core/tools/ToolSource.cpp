#include "core/tools/ToolSource.h"

#include <algorithm>
#include <format>
#include <regex>
#include <string>
#include <utility>

namespace westie::core {

namespace {

constexpr std::string_view YtDlpAsset = "yt-dlp.exe";
const std::regex FfmpegBranchBuild(R"(ffmpeg-n(\d+)\.(\d+)-latest-win64-gpl-\1\.\2\.zip)");

std::optional<ToolDownload> choose_yt_dlp(const Release& release) {
    auto asset = std::ranges::find(release.assets, YtDlpAsset, &ReleaseAsset::name);
    if (asset == release.assets.end()) {
        return std::nullopt;
    }
    return ToolDownload{.release = release.tag, .asset = *asset};
}

std::optional<ToolDownload> choose_ffmpeg(const Release& release) {
    // BtbN's latest release has a build of each release branch, and the newest branch wins. The builds are remade
    // daily, so the branch, not the tag, names the release.
    std::optional<ToolDownload> newest;
    std::pair<int, int> newest_branch{-1, -1};
    for (const ReleaseAsset& asset : release.assets) {
        std::smatch match;
        if (!std::regex_match(asset.name, match, FfmpegBranchBuild)) {
            continue;
        }
        std::pair branch{std::stoi(match[1].str()), std::stoi(match[2].str())};
        if (branch > newest_branch) {
            newest_branch = branch;
            newest = ToolDownload{.release = std::format("n{}.{}", branch.first, branch.second), .asset = asset, .archive = true};
        }
    }
    return newest;
}

}

std::string_view tool_repository(ToolPackage package) noexcept {
    switch (package) {
    case ToolPackage::YtDlp:  return "yt-dlp/yt-dlp";
    case ToolPackage::Ffmpeg: return "BtbN/FFmpeg-Builds";
    }
    return {};
}

std::optional<ToolDownload> choose_download(ToolPackage package, const Release& release) {
    switch (package) {
    case ToolPackage::YtDlp:  return choose_yt_dlp(release);
    case ToolPackage::Ffmpeg: return choose_ffmpeg(release);
    }
    return std::nullopt;
}

}
