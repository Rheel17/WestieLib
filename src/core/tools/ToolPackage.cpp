#include "core/tools/ToolPackage.h"

#include <algorithm>

namespace westie::core {

namespace {

constexpr std::array<std::wstring_view, 1> YtDlpFiles = {L"yt-dlp.exe"};
constexpr std::array<std::wstring_view, 2> FfmpegFiles = {L"ffmpeg.exe", L"ffprobe.exe"};

}

std::string_view tool_package_name(ToolPackage package) noexcept {
    switch (package) {
    case ToolPackage::YtDlp:  return "yt-dlp";
    case ToolPackage::Ffmpeg: return "ffmpeg";
    }
    return {};
}

std::optional<ToolPackage> tool_package_named(std::string_view name) noexcept {
    auto found = std::ranges::find(AllToolPackages, name, tool_package_name);
    if (found == AllToolPackages.end()) {
        return std::nullopt;
    }
    return *found;
}

std::span<const std::wstring_view> tool_package_files(ToolPackage package) noexcept {
    switch (package) {
    case ToolPackage::YtDlp:  return YtDlpFiles;
    case ToolPackage::Ffmpeg: return FfmpegFiles;
    }
    return {};
}

}
