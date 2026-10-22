#pragma once

#include <string>

namespace westie::core {

struct PlaylistFetchFailed {
    std::string playlist{};
    std::string error{};
};

}
