#pragma once

#include <filesystem>
#include <string>

#include "platform/Hasher.h"

namespace westie::platform::win32 {

class BCryptHasher final : public Hasher {
public:
    std::string sha256(const std::filesystem::path& file) const override;
};

}
