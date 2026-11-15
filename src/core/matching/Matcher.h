#pragma once

#include <expected>
#include <set>
#include <stop_token>
#include <string>

#include "core/log/Log.h"
#include "core/log/LogSubject.h"
#include "core/state/Match.h"
#include "core/state/Source.h"
#include "platform/HttpClient.h"
#include "platform/TextNormalizer.h"

namespace westie::core {

class Matcher {
public:
    Matcher(platform::HttpClient& http, const platform::TextNormalizer& normalizer, std::string api_url, Log& log);

    std::expected<Match, std::string> match(
            const Source& source, const std::set<std::string>& known_ytm_ids, const LogSubject& subject, const std::stop_token& stop
    ) const;

private:
    platform::HttpClient& http_;
    const platform::TextNormalizer& normalizer_;
    std::string api_url_;
    Log& log_;
};

}
