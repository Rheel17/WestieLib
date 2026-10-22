#pragma once

#include <exception>

namespace westie::platform {

class Cancelled final : public std::exception {
public:
    const char* what() const noexcept override { return "Cancelled"; }
};

}
