#include "ui/DurationText.h"

#include <format>

namespace westie::ui {

namespace {

constexpr int SecondsPerMinute = 60;

}

std::wstring duration_text(std::chrono::milliseconds duration) {
    auto seconds = std::chrono::duration_cast<std::chrono::seconds>(duration).count();
    return std::format(L"{}:{:02}", seconds / SecondsPerMinute, seconds % SecondsPerMinute);
}

}
