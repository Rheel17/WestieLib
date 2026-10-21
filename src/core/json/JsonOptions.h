#pragma once

#include <cstdint>

#include <glaze/glaze.hpp>

namespace westie::core {

// Files from a later version may have keys this one doesn't know
constexpr glz::opts JsonReadOptions{.error_on_unknown_keys = false};

struct JsonWriteOptions : glz::opts {
    std::uint8_t indentation_width = 4;
};

constexpr JsonWriteOptions PrettyJsonWriteOptions = [] {
    JsonWriteOptions options;
    options.prettify = true;
    return options;
}();

}
