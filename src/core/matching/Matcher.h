#pragma once

#include <expected>
#include <set>
#include <stop_token>
#include <string>

#include "core/log/Log.h"
#include "core/log/LogSubject.h"
#include "core/matching/YouTubeMusicSearch.h"
#include "core/state/Match.h"
#include "core/state/Source.h"
#include "platform/TextNormalizer.h"

namespace westie::core {

class Matcher {
public:
    Matcher(YouTubeMusicSearch& search, const platform::TextNormalizer& normalizer, Log& log);

    std::expected<Match, std::string> match(
            const Source& source, const std::set<std::string>& known_ytm_ids, const std::set<std::string>& unavailable_ytm_ids, const LogSubject& subject,
            const std::stop_token& stop
    ) const;

private:
    YouTubeMusicSearch& search_;
    const platform::TextNormalizer& normalizer_;
    Log& log_;
};

}
