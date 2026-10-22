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
};

std::optional<std::string_view> tool_repository(ToolPackage package) noexcept;
std::optional<ToolDownload> choose_download(ToolPackage package, const Release& release);

}
