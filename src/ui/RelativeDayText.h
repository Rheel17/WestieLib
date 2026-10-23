#pragma once

#include <chrono>
#include <string>

namespace westie::ui {

std::wstring relative_day_text(std::chrono::local_seconds when, std::chrono::local_seconds now);
// In the PC's time zone
std::wstring relative_day_text(std::chrono::system_clock::time_point when, std::chrono::system_clock::time_point now);

}
