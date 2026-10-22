#pragma once

#include <filesystem>
#include <span>
#include <stop_token>
#include <string_view>

namespace westie::platform {

class ArchiveExtractor {
public:
    virtual ~ArchiveExtractor() = default;

    virtual void extract(
            const std::filesystem::path& archive, std::span<const std::wstring_view> file_names, const std::filesystem::path& folder,
            const std::stop_token& stop
    ) const = 0;
};

}
