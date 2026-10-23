#pragma once

#include <format>
#include <stdexcept>
#include <string_view>

namespace westie::platform {

class HttpError final : public std::runtime_error {
public:
    HttpError(int status, std::string_view host) : std::runtime_error(std::format("HTTP {} from {}", status, host)), status_(status) {}

    int status() const noexcept { return status_; }

private:
    int status_;
};

}
