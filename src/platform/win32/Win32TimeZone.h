#pragma once

#include <chrono>

#include "platform/TimeZone.h"

namespace westie::platform::win32 {

class Win32TimeZone final : public TimeZone {
public:
    std::chrono::local_time<std::chrono::milliseconds> to_local(std::chrono::system_clock::time_point time) const override;
};

}
