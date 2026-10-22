#pragma once

#include <cstdint>
#include <string>
#include <vector>

namespace westie::core {

struct ReleaseAsset {
    std::string name{};
    std::string url{};
    std::uint64_t size = 0;
    // "sha256:<hex>", or empty for assets uploaded before GitHub kept digests
    std::string digest{};
};

struct Release {
    std::string tag{};
    std::vector<ReleaseAsset> assets{};
};

}
