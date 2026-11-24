#pragma once

#include <filesystem>
#include <memory>
#include <optional>
#include <string>
#include <string_view>
#include <vector>

#include "platform/AppendFile.h"

namespace westie::platform {

class FileOps {
public:
    virtual ~FileOps() = default;

    virtual bool is_file(const std::filesystem::path& file) const = 0;
    virtual bool exists(const std::filesystem::path& path) const = 0;
    virtual std::optional<std::string> read_file(const std::filesystem::path& file) const = 0;
    virtual void replace_file(const std::filesystem::path& file, std::string_view contents) = 0;
    virtual void replace_file_keeping_backup(const std::filesystem::path& file, const std::filesystem::path& backup, std::string_view contents) = 0;
    virtual void create_folders(const std::filesystem::path& folder) = 0;
    virtual void remove_tree(const std::filesystem::path& path) = 0;
    virtual void move_replacing(const std::filesystem::path& from, const std::filesystem::path& to) = 0;
    // False when the name is taken
    virtual bool move_new(const std::filesystem::path& from, const std::filesystem::path& to) = 0;
    virtual std::unique_ptr<AppendFile> open_for_append(const std::filesystem::path& file) = 0;
    virtual std::vector<std::filesystem::path> list_files(const std::filesystem::path& folder) const = 0;
    virtual void remove_file(const std::filesystem::path& file) = 0;
};

}
