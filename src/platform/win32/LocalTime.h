#pragma once

#include <chrono>

namespace westie::platform::win32 {

std::chrono::local_seconds to_local_time(std::chrono::system_clock::time_point time);

}
