#pragma once

#include <string>
#include <string_view>

#include "platform/TextNormalizer.h"

namespace westie::platform::win32 {

class Win32TextNormalizer final : public TextNormalizer {
public:
    std::string normalize(std::string_view text) const override;
};

}
