#include "core/matching/RefusedSearch.h"

#include <algorithm>
#include <array>

namespace westie::core {

namespace {

// What YouTube Music answers a search it won't serve right now: Forbidden, Too Many Requests
constexpr std::array RefusedStatuses = {403, 429};

}

bool refused_search(int status) noexcept {
    return std::ranges::contains(RefusedStatuses, status);
}

}
