#pragma once

#include <chrono>
#include <format>
#include <stdexcept>
#include <string_view>

namespace westie::platform {

class HttpTimeout final : public std::runtime_error {
public:
    HttpTimeout(std::string_view host, std::chrono::milliseconds timeout) :
            std::runtime_error(std::format("No answer from {} within {} ms", host, timeout.count())) {}
};

}
