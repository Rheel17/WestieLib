#pragma once

#include <filesystem>
#include <optional>
#include <string>
#include <string_view>

#include "platform/FileOps.h"

namespace westie::platform::win32 {

class Win32FileOps final : public FileOps {
public:
    bool is_file(const std::filesystem::path& file) const override;
    std::optional<std::string> read_file(const std::filesystem::path& file) const override;
    void replace_file(const std::filesystem::path& file, std::string_view contents) override;
};

}
