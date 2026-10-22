#pragma once

#include <filesystem>
#include <optional>
#include <string>
#include <string_view>

namespace westie::platform {

class FileOps {
public:
    virtual ~FileOps() = default;

    virtual bool is_file(const std::filesystem::path& file) const = 0;
    virtual std::optional<std::string> read_file(const std::filesystem::path& file) const = 0;
    virtual void replace_file(const std::filesystem::path& file, std::string_view contents) = 0;
    virtual void replace_file_keeping_backup(const std::filesystem::path& file, const std::filesystem::path& backup, std::string_view contents) = 0;
    virtual void create_folders(const std::filesystem::path& folder) = 0;
    virtual void remove_tree(const std::filesystem::path& path) = 0;
    virtual void move_replacing(const std::filesystem::path& from, const std::filesystem::path& to) = 0;
};

}
