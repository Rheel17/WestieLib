#pragma once

#include <chrono>

namespace westie::platform {

class TimeZone {
public:
    virtual ~TimeZone() = default;

    virtual std::chrono::local_time<std::chrono::milliseconds> to_local(std::chrono::system_clock::time_point time) const = 0;
};

}
