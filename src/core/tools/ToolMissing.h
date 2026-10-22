#pragma once

#include <format>
#include <stdexcept>

#include "core/tools/ToolPackage.h"

namespace westie::core {

class ToolMissing final : public std::runtime_error {
public:
    explicit ToolMissing(ToolPackage package) : std::runtime_error(std::format("{} isn't installed", tool_package_name(package))) {}
};

}
