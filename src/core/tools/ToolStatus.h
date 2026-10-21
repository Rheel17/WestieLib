#pragma once

#include <string>

#include "core/tools/ToolPackage.h"

namespace westie::core {

enum class ToolState { Missing, Installed, Checking, Downloading, Installing, UpToDate, Failed };

struct ToolStatus {
    ToolPackage package = ToolPackage::YtDlp;
    ToolState state = ToolState::Missing;
    // Empty while the tool isn't installed; a failed update keeps the release still in place
    std::string release{};
    int percent = 0;
    std::string error{};
};

}
