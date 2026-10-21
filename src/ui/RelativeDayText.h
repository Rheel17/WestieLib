#pragma once

#include <chrono>
#include <string>

namespace westie::ui {

std::wstring relative_day_text(std::chrono::local_seconds when, std::chrono::local_seconds now);

}
