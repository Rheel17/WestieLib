#pragma once

#include <stop_token>
#include <string>
#include <string_view>

#include "core/github/Release.h"
#include "platform/HttpClient.h"

namespace westie::core {

class GitHubReleases {
public:
    GitHubReleases(platform::HttpClient& http, std::string api_url);

    Release latest(std::string_view repository, const std::stop_token& stop) const;

private:
    platform::HttpClient& http_;
    std::string api_url_;
};

}
