#pragma once

#include <chrono>
#include <string>

namespace westie::ui {

std::wstring duration_text(std::chrono::milliseconds duration);

}
