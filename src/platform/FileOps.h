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
};

}
