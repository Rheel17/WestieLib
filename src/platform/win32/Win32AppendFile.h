#pragma once

#include <filesystem>
#include <string_view>

#include "platform/AppendFile.h"
#include "platform/win32/UniqueHandle.h"

namespace westie::platform::win32 {

class Win32AppendFile final : public AppendFile {
public:
    explicit Win32AppendFile(const std::filesystem::path& file);

    void append(std::string_view text) override;

private:
    UniqueFile handle_;
};

}
