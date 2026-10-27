#pragma once

#include <expected>
#include <set>
#include <stop_token>
#include <string>

#include "core/state/Match.h"
#include "core/state/Source.h"
#include "platform/HttpClient.h"
#include "platform/TextNormalizer.h"

namespace westie::core {

class Matcher {
public:
    Matcher(platform::HttpClient& http, const platform::TextNormalizer& normalizer, std::string api_url);

    std::expected<Match, std::string> match(const Source& source, const std::set<std::string>& known_videos, const std::stop_token& stop) const;

private:
    platform::HttpClient& http_;
    const platform::TextNormalizer& normalizer_;
    std::string api_url_;
};

}
