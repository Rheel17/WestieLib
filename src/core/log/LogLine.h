#pragma once

#include <chrono>
#include <string>

#include "core/log/LogLevel.h"
#include "core/log/LogSubject.h"

namespace westie::core {

struct LogLine {
    std::chrono::local_time<std::chrono::milliseconds> time{};
    LogLevel level = LogLevel::Info;
    std::string text{};
    LogSubject subject{};
};

}
