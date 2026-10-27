#pragma once

#include <string>
#include <string_view>

namespace westie::platform {

class TextNormalizer {
public:
    virtual ~TextNormalizer() = default;

    virtual std::string normalize(std::string_view text) const = 0;
};

}
