#pragma once

#include <optional>
#include <string>
#include <string_view>

#include "core/github/Release.h"
#include "core/tools/ToolPackage.h"

namespace westie::core {

struct ToolDownload {
    std::string release{};
    ReleaseAsset asset{};
    // A zip that holds the tool's files, rather than the one file itself
    bool archive = false;
};

std::string_view tool_repository(ToolPackage package) noexcept;
std::optional<ToolDownload> choose_download(ToolPackage package, const Release& release);

}
