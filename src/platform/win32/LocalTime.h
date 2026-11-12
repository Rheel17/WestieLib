#pragma once

#include <chrono>

namespace westie::platform::win32 {

std::chrono::local_time<std::chrono::milliseconds> to_local_milliseconds(std::chrono::system_clock::time_point time);
std::chrono::local_seconds to_local_time(std::chrono::system_clock::time_point time);

}
