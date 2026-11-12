#pragma once

#include <string_view>

namespace westie::platform {

class AppendFile {
public:
    virtual ~AppendFile() = default;

    virtual void append(std::string_view text) = 0;
};

}
