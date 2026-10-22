#pragma once

#include <filesystem>
#include <string>

namespace westie::platform {

class Hasher {
public:
    virtual ~Hasher() = default;

    virtual std::string sha256(const std::filesystem::path& file) const = 0;
};

}
