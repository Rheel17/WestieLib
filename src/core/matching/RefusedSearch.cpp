#include "core/matching/RefusedSearch.h"

#include <algorithm>
#include <array>

#include "platform/HttpError.h"
#include "platform/HttpTimeout.h"

namespace westie::core {

namespace {

// What YouTube Music answers a search it won't serve right now: Forbidden, Too Many Requests
constexpr std::array RefusedStatuses = {403, 429};

}

bool refused_search(const std::exception& error) noexcept {
    if (dynamic_cast<const platform::HttpTimeout*>(&error)) {
        return true;
    }
    auto http_error = dynamic_cast<const platform::HttpError*>(&error);
    return http_error && std::ranges::contains(RefusedStatuses, http_error->status());
}

}
