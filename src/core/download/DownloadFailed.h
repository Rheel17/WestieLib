#pragma once

#include <stdexcept>
#include <string>

namespace westie::core {

class DownloadFailed final : public std::runtime_error {
public:
    explicit DownloadFailed(const std::string& error) : std::runtime_error(error) {}
};

}
