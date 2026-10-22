#pragma once

#include <filesystem>
#include <span>
#include <stop_token>
#include <string_view>

#include "platform/ArchiveExtractor.h"

namespace westie::platform::win32 {

class MinizArchiveExtractor final : public ArchiveExtractor {
public:
    void extract(
            const std::filesystem::path& archive,
            std::span<const std::wstring_view> file_names,
            const std::filesystem::path& folder,
            const std::stop_token& stop
    ) const override;
};

}
