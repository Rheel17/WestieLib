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
    void create_folders(const std::filesystem::path& folder) override;
    void remove_tree(const std::filesystem::path& path) override;
    void move_replacing(const std::filesystem::path& from, const std::filesystem::path& to) override;
};

}
