#include "ui/RelativeDayText.h"

#include <array>
#include <format>

#include "platform/win32/LocalTime.h"

namespace westie::ui {

namespace {

constexpr std::array<const wchar_t*, 12> MonthNames = {L"Jan", L"Feb", L"Mar", L"Apr", L"May", L"Jun", L"Jul", L"Aug", L"Sep", L"Oct", L"Nov", L"Dec"};

}

std::wstring relative_day_text(std::chrono::local_seconds when, std::chrono::local_seconds now) {
    std::chrono::local_days day = std::chrono::floor<std::chrono::days>(when);
    std::chrono::local_days today = std::chrono::floor<std::chrono::days>(now);
    if (day == today) {
        std::chrono::hh_mm_ss time{std::chrono::floor<std::chrono::minutes>(when - day)};
        return std::format(L"today {:02}:{:02}", time.hours().count(), time.minutes().count());
    }
    if (day == today - std::chrono::days(1)) {
        return L"yesterday";
    }

    std::chrono::year_month_day date{day};
    std::chrono::year_month_day current{today};
    const wchar_t* month = MonthNames[static_cast<unsigned>(date.month()) - 1];
    if (date.year() == current.year()) {
        return std::format(L"{} {}", static_cast<unsigned>(date.day()), month);
    }
    return std::format(L"{} {} {}", static_cast<unsigned>(date.day()), month, static_cast<int>(date.year()));
}

std::wstring relative_day_text(std::chrono::system_clock::time_point when, std::chrono::system_clock::time_point now) {
    return relative_day_text(platform::win32::to_local_time(when), platform::win32::to_local_time(now));
}

}
