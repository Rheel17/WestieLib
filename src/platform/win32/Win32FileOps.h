#pragma once

#include <filesystem>
#include <memory>
#include <optional>
#include <string>
#include <string_view>
#include <vector>

#include "platform/FileOps.h"

namespace westie::platform::win32 {

class Win32FileOps final : public FileOps {
public:
    bool is_file(const std::filesystem::path& file) const override;
    std::optional<std::string> read_file(const std::filesystem::path& file) const override;
    void replace_file(const std::filesystem::path& file, std::string_view contents) override;
    void replace_file_keeping_backup(const std::filesystem::path& file, const std::filesystem::path& backup, std::string_view contents) override;
    void create_folders(const std::filesystem::path& folder) override;
    void remove_tree(const std::filesystem::path& path) override;
    void move_replacing(const std::filesystem::path& from, const std::filesystem::path& to) override;
    std::unique_ptr<AppendFile> open_for_append(const std::filesystem::path& file) override;
    std::vector<std::filesystem::path> list_files(const std::filesystem::path& folder) const override;
    void remove_file(const std::filesystem::path& file) override;
};

}
