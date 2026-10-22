#include "core/github/GitHubReleases.h"

#include <cstdint>
#include <format>
#include <optional>
#include <stdexcept>
#include <utility>
#include <vector>

#include <glaze/glaze.hpp>

#include "core/json/JsonOptions.h"

namespace westie::core {

namespace {

// The parts of GitHub's release object the app uses
struct AssetJson {
    std::string name{};
    std::string browser_download_url{};
    std::uint64_t size = 0;
    std::optional<std::string> digest{};
};

struct ReleaseJson {
    std::string tag_name{};
    std::vector<AssetJson> assets{};
};

}

GitHubReleases::GitHubReleases(platform::HttpClient& http, std::string api_url) : http_(http), api_url_(std::move(api_url)) {}

Release GitHubReleases::latest(std::string_view repository, const std::stop_token& stop) const {
    std::string body = http_.get(std::format("{}/repos/{}/releases/latest", api_url_, repository), stop);
    ReleaseJson json;
    if (glz::error_ctx error = glz::read<JsonReadOptions>(json, body)) {
        throw std::runtime_error(std::format("GitHub's answer for {} doesn't read: {}", repository, glz::format_error(error, body)));
    }

    Release release{.tag = std::move(json.tag_name)};
    for (AssetJson& asset : json.assets) {
        release.assets.push_back({
            .name = std::move(asset.name),
            .url = std::move(asset.browser_download_url),
            .size = asset.size,
            .digest = std::move(asset.digest).value_or(""),
        });
    }
    return release;
}

}
