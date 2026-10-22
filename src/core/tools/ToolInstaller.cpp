#include "core/tools/ToolInstaller.h"

#include <algorithm>
#include <cstdint>
#include <format>
#include <stdexcept>
#include <string>
#include <utility>

#include "core/json/Utf8Path.h"

namespace westie::core {

namespace {

constexpr int FullPercent = 100;

}

ToolInstaller::ToolInstaller(platform::FileOps& files, platform::HttpClient& http, const platform::Hasher& hasher, std::filesystem::path folder) :
        files_(files), http_(http), hasher_(hasher), folder_(std::move(folder)) {}

InstalledTool ToolInstaller::install(ToolPackage package, const ToolDownload& download, const Progress& report, const std::stop_token& stop) {
    const ReleaseAsset& asset = download.asset;
    if (asset.digest.empty()) {
        throw std::runtime_error(std::format("{} has no digest to check the download against", asset.name));
    }

    // The asset's name comes from the network, so it doesn't name the file
    std::filesystem::path staging = staging_folder() / path_from_utf8(tool_package_name(package));
    std::filesystem::path file = staging / L"download";
    files_.remove_tree(staging);
    files_.create_folders(staging);

    int shown = 0;
    report(ToolState::Downloading, shown);
    auto on_progress = [&](std::uint64_t received) {
        int percent = asset.size == 0 ? 0 : static_cast<int>(std::min<std::uint64_t>(FullPercent, received * FullPercent / asset.size));
        if (percent != shown) {
            shown = percent;
            report(ToolState::Downloading, percent);
        }
    };
    http_.download(asset.url, file, on_progress, stop);

    report(ToolState::Installing, FullPercent);
    std::string digest = "sha256:" + hasher_.sha256(file);
    if (digest != asset.digest) {
        throw std::runtime_error(std::format("{} doesn't match its digest", asset.name));
    }
    files_.move_replacing(file, folder_ / tool_package_files(package).front());
    files_.remove_tree(staging);
    return {.release = download.release, .digest = std::move(digest)};
}

void ToolInstaller::clear_staging() {
    files_.remove_tree(staging_folder());
}

std::filesystem::path ToolInstaller::staging_folder() const {
    return folder_ / L".staging";
}

}
