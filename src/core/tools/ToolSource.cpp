#include "core/tools/ToolSource.h"

#include <algorithm>

namespace westie::core {

namespace {

constexpr std::string_view YtDlpAsset = "yt-dlp.exe";

}

std::optional<std::string_view> tool_repository(ToolPackage package) noexcept {
    switch (package) {
    case ToolPackage::YtDlp:  return "yt-dlp/yt-dlp";
    case ToolPackage::Ffmpeg: return std::nullopt;
    }
    return std::nullopt;
}

std::optional<ToolDownload> choose_download(ToolPackage package, const Release& release) {
    if (package != ToolPackage::YtDlp) {
        return std::nullopt;
    }
    auto asset = std::ranges::find(release.assets, YtDlpAsset, &ReleaseAsset::name);
    if (asset == release.assets.end()) {
        return std::nullopt;
    }
    return ToolDownload{.release = release.tag, .asset = *asset};
}

}
