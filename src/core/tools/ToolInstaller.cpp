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

ToolInstaller::ToolInstaller(const ToolServices& services, std::filesystem::path folder) : services_(services), folder_(std::move(folder)) {}

InstalledTool ToolInstaller::install(ToolPackage package, const ToolDownload& download, const Progress& report, const std::stop_token& stop) {
    const ReleaseAsset& asset = download.asset;
    if (asset.digest.empty()) {
        throw std::runtime_error(std::format("{} has no digest to check the download against", asset.name));
    }

    // The asset's name comes from the network, so it doesn't name the file
    std::filesystem::path staging = staging_folder() / path_from_utf8(tool_package_name(package));
    std::filesystem::path file = staging / L"download";
    services_.files.remove_tree(staging);
    services_.files.create_folders(staging);
    fetch(asset, file, report, stop);

    report(ToolState::Installing, FullPercent);
    std::string digest = "sha256:" + services_.hasher.sha256(file);
    if (digest != asset.digest) {
        throw std::runtime_error(std::format("{} doesn't match its digest", asset.name));
    }
    if (download.archive) {
        std::filesystem::path unpacked = staging / L"unpacked";
        services_.files.create_folders(unpacked);
        services_.archives.extract(file, tool_package_files(package), unpacked, stop);
        for (std::wstring_view name : tool_package_files(package)) {
            services_.files.move_replacing(unpacked / name, folder_ / name);
        }
    } else {
        services_.files.move_replacing(file, folder_ / tool_package_files(package).front());
    }
    services_.files.remove_tree(staging);
    return {.release = download.release, .digest = std::move(digest)};
}

void ToolInstaller::clear_staging() {
    services_.files.remove_tree(staging_folder());
}

void ToolInstaller::fetch(const ReleaseAsset& asset, const std::filesystem::path& file, const Progress& report, const std::stop_token& stop) {
    int shown = 0;
    report(ToolState::Downloading, shown);
    auto on_progress = [&](std::uint64_t received) {
        int percent = asset.size == 0 ? 0 : static_cast<int>(std::min<std::uint64_t>(FullPercent, received * FullPercent / asset.size));
        if (percent != shown) {
            shown = percent;
            report(ToolState::Downloading, percent);
        }
    };
    services_.http.download(asset.url, file, on_progress, stop);
}

std::filesystem::path ToolInstaller::staging_folder() const {
    return folder_ / L".staging";
}

}
